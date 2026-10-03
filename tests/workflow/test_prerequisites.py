"""Capability-selection and bounded failure receipts, with no native suite execution."""
import importlib.util
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location("check_prerequisites", Path(__file__).resolve().parents[2] / "scripts/check_prerequisites.py")
prerequisites = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(prerequisites)

RUNNER_SPEC = importlib.util.spec_from_file_location("run_tests", Path(__file__).resolve().parents[2] / "scripts/run_tests.py")
runner = importlib.util.module_from_spec(RUNNER_SPEC)
with patch.dict(sys.modules, {"check_prerequisites": prerequisites}):
    RUNNER_SPEC.loader.exec_module(runner)


class PrerequisiteTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.build = Path(self.directory.name)
        self.cache()
        self.calls = []

    def cache(self, sanitize=False):
        (self.build / "CMakeCache.txt").write_text("CMAKE_CXX_COMPILER:FILEPATH=/configured/compiler\nCMAKE_CXX_FLAGS:STRING=-stdlib=libc++\nCRUCIBLE_ENABLE_SANITIZERS:BOOL=" + ("ON" if sanitize else "OFF") + "\n")

    def probe(self, name, command, *, timeout, env):
        self.calls.append((name, command, timeout, env))
        output = "cmake version 3.25.0" if name == "cmake" else ""
        if name == "vulkan":
            output = "GPU0:\n deviceName = llvmpipe (LLVM 19)\n deviceType = PHYSICAL_DEVICE_TYPE_CPU\n"
        return {"status": "pass", "returncode": 0, "output": output}

    def check(self, tests=None, **options):
        return prerequisites.check_prerequisites(self.build, [] if tests is None else tests,
                                                 probe=self.probe, env={"DISPLAY": ":99"}, **options)

    def gpu_tests(self, name="gpu_instanced_readback"):
        shaders = []
        for filename in ("world.vert.spv", "world.frag.spv"):
            shader = self.build / filename
            shader.write_bytes(b"\x03\x02\x23\x07payload")
            shaders.append(str(shader))
        return [{"name": name, "command": ["/native/receiver", *shaders]}]

    def test_portable_gpu_selection_has_no_display_or_vulkan_probe(self):
        receipt = self.check([{"name": "gpu_numeric"}, {"name": "workflow_permission_repair"}])
        self.assertTrue(receipt["ok"])
        self.assertFalse(receipt["gpu_required"])
        self.assertEqual([], self.calls)

    def test_selected_receiver_requires_display_vulkan_and_declared_shaders(self):
        for name in prerequisites.GPU_TESTS:
            with self.subTest(name=name):
                receipt = self.check(self.gpu_tests(name))
                self.assertTrue(receipt["ok"])
                self.assertEqual(["display", "vulkan"], [row["name"] for row in receipt["checks"] if row["name"] in {"display", "vulkan"}])
                self.assertEqual(2, len([row for row in receipt["checks"] if row["name"] == "spirv"]))

    def test_software_gpu_passes_correctness_but_fails_hardware_requirement(self):
        self.assertTrue(self.check(self.gpu_tests())["ok"])
        receipt = self.check(self.gpu_tests(), require_hardware=True)
        self.assertFalse(receipt["ok"])
        hardware = next(row for row in receipt["checks"] if row["name"] == "hardware_gpu")
        self.assertEqual("fail", hardware["status"])
        self.assertEqual("PHYSICAL_DEVICE_TYPE_CPU", hardware["devices"][0]["type"])

    def test_discrete_report_has_honest_scope(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            if name == "vulkan":
                result["output"] = "GPU0:\n deviceName = AMD RX\n deviceType = PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n"
            return result
        receipt = prerequisites.check_prerequisites(self.build, self.gpu_tests(), require_hardware=True,
                                                     probe=probe, env={"DISPLAY": ":99"})
        self.assertTrue(receipt["ok"])
        self.assertIn("no physical readback", next(row["scope"] for row in receipt["checks"] if row["name"] == "hardware_gpu"))

    def test_dummy_backend_cannot_satisfy_video_requirement(self):
        receipt = prerequisites.check_prerequisites(self.build, self.gpu_tests(), probe=self.probe,
                                                     env={"DISPLAY": ":99", "SDL_VIDEO_DRIVER": "dummy", "SDL_VIDEODRIVER": "x11"})
        self.assertFalse(receipt["ok"])
        self.assertEqual("unknown", next(row["status"] for row in receipt["checks"] if row["name"] != "python"))
        self.assertNotIn("display", [call[0] for call in self.calls])

    def test_missing_shader_blocks_and_receipt_serializes(self):
        tests = self.gpu_tests()
        Path(tests[0]["command"][1]).unlink()
        receipt = self.check(tests)
        self.assertFalse(receipt["ok"])
        self.assertEqual(receipt, json.loads(json.dumps(receipt)))
        self.assertIsNone(receipt["capacity"]["memory_bytes"])

    def test_missing_tool_and_old_cmake_block_development(self):
        for problem in ("missing", "old", "unknown"):
            def probe(name, command, **kwargs):
                result = self.probe(name, command, **kwargs)
                if name == "cmake":
                    result = {"status": "fail" if problem == "missing" else "pass",
                              "output": "missing tool" if problem == "missing" else "cmake version 3.24.9" if problem == "old" else "unrecognized"}
                return result
            with self.subTest(problem=problem):
                receipt = prerequisites.check_prerequisites(self.build, stage="development", probe=probe, env={})
                self.assertFalse(receipt["ok"])

    def test_compiler_capability_respects_cache_and_explicit_override(self):
        self.check(stage="development")
        compile_call = next(call for call in self.calls if call[0] == "compiler_canary")
        self.assertEqual("/configured/compiler", compile_call[1][0])
        self.assertIn("-stdlib=libc++", compile_call[1])
        self.calls.clear()
        self.check(stage="development", compiler="/explicit/compiler")
        self.assertEqual("/explicit/compiler", next(call[1][0] for call in self.calls if call[0] == "compiler_canary"))

    def test_sanitizer_runtime_failure_or_unknown_blocks_without_disabling_leaks(self):
        self.cache(sanitize=True)
        for status in ("fail", "unknown"):
            def probe(name, command, **kwargs):
                result = self.probe(name, command, **kwargs)
                return {"status": status, "output": "LSan /proc namespace unavailable"} if name == "sanitizer_canary" else result
            receipt = prerequisites.check_prerequisites(self.build, [], probe=probe,
                                                         env={"ASAN_OPTIONS": "detect_leaks=0:detect_leaks=1"})
            self.assertFalse(receipt["ok"])
            call = next(call for call in reversed(self.calls) if call[0] == "sanitizer_canary")
            self.assertTrue(call[3]["ASAN_OPTIONS"].endswith("detect_leaks=1"))
            self.assertTrue(call[3]["LSAN_OPTIONS"].endswith("leak_check_at_exit=1"))
            self.assertLessEqual(call[2], 10)

    def test_compiled_development_canary_runtime_failure_blocks(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            return {"status": "fail", "output": "bundled standard library failed to load"} if name == "runtime_canary" else result
        receipt = prerequisites.check_prerequisites(self.build, stage="development", probe=probe, env={})
        self.assertFalse(receipt["ok"])
        self.assertEqual(["compiler_canary", "runtime_canary"],
                         [call[0] for call in self.calls if "canary" in call[0]])
        self.assertEqual("fail", next(row["status"] for row in receipt["checks"] if row["name"] == "runtime_canary"))

    def test_compile_failure_never_executes_sanitizer_canary(self):
        self.cache(sanitize=True)
        def probe(name, command, **kwargs):
            self.calls.append((name, command, kwargs))
            return {"status": "fail", "output": "std::expected unavailable"}
        receipt = prerequisites.check_prerequisites(self.build, [], probe=probe, env={})
        self.assertFalse(receipt["ok"])
        self.assertEqual(["compiler_canary"], [call[0] for call in self.calls])
        self.assertEqual("unknown", receipt["checks"][-1]["status"])

    def test_missing_configured_compiler_blocks_even_portable_selection(self):
        (self.build / "CMakeCache.txt").write_text("CRUCIBLE_ENABLE_SANITIZERS:BOOL=OFF\n")
        self.assertFalse(self.check()["ok"])
        self.assertEqual([], self.calls)

    def test_vulkan_summary_without_device_type_cannot_claim_hardware(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            if name == "vulkan":
                result["output"] = "GPU0:\n deviceName = unidentified GPU\n"
            return result
        receipt = prerequisites.check_prerequisites(self.build, self.gpu_tests(), require_hardware=True,
                                                     probe=probe, env={"DISPLAY": ":99"})
        self.assertFalse(receipt["ok"])

    def test_exhausted_overall_budget_blocks_without_launch(self):
        from unittest.mock import patch
        with patch.object(prerequisites.time, "monotonic", side_effect=[0] + [100] * 12):
            receipt = prerequisites.check_prerequisites(self.build, stage="development", probe=self.probe,
                                                         env={}, budget=1)
        self.assertFalse(receipt["ok"])
        self.assertEqual([], self.calls)
        self.assertEqual("unknown", next(row["status"] for row in receipt["checks"] if row["name"] != "python"))

    def test_missing_inventory_is_unknown_and_blocked(self):
        receipt = prerequisites.check_prerequisites(self.build, probe=self.probe)
        self.assertFalse(receipt["ok"])
        self.assertEqual("selected_inventory", receipt["checks"][-1]["name"])

    def test_probe_timeout_kills_and_reaps_group_with_receipt(self):
        from unittest.mock import patch, Mock
        child = Mock(pid=12345, returncode=-9, stdout=io.BytesIO())
        child.wait.side_effect = [subprocess.TimeoutExpired("tool", 0.2), -9]
        with patch.object(prerequisites.subprocess, "Popen", return_value=child), \
                patch.object(prerequisites.os, "killpg") as kill:
            receipt = prerequisites.run_probe("tool", ["tool"], timeout=0.2, env={})
        self.assertEqual("unknown", receipt["status"])
        self.assertIn("timed out", receipt["output"])
        kill.assert_called_once_with(12345, prerequisites.signal.SIGKILL)
        self.assertEqual(2, child.wait.call_count)
        self.assertEqual(["tool"], receipt["command"])
        self.assertIn("elapsed_seconds", receipt)

    def test_probe_output_receipt_is_bounded(self):
        from unittest.mock import patch, Mock
        def launch(command, **options):
            return Mock(wait=Mock(return_value=0), stdout=io.BytesIO(b"x" * 10000))
        with patch.object(prerequisites.subprocess, "Popen", side_effect=launch):
            receipt = prerequisites.run_probe("tool", ["tool"], timeout=1, env={})
        self.assertEqual(6000, len(receipt["output"]))
        self.assertTrue(receipt["output_truncated"])

    def test_effective_leak_disable_blocks_before_compiler(self):
        self.cache(sanitize=True)
        for variable, value in [("ASAN_OPTIONS", "detect_leaks=1:detect_leaks=0"),
                                ("LSAN_OPTIONS", "leak_check_at_exit=false"),
                                ("LSAN_OPTIONS", "detect_leaks=false")]:
            with self.subTest(variable=variable, value=value):
                receipt = prerequisites.check_prerequisites(self.build, [], probe=self.probe,
                                                             env={variable: value})
                self.assertFalse(receipt["ok"])
                self.assertEqual("sanitizer_environment", receipt["checks"][-1]["name"])
        self.assertEqual([], self.calls)

    def test_gpu_development_needs_shader_tool_but_no_display(self):
        receipt = prerequisites.check_prerequisites(self.build, stage="development", gpu_requested=True,
                                                     probe=self.probe, env={})
        self.assertTrue(receipt["ok"])
        names = [call[0] for call in self.calls]
        self.assertIn("shader_compiler", names)
        self.assertIn("vulkan", names)
        self.assertNotIn("display", names)
        self.calls.clear()
        receipt = self.check([{"name": "gpu_numeric"}], gpu_requested=True)
        self.assertTrue(receipt["ok"])
        self.assertEqual([], self.calls)

    def test_gpu_development_missing_shader_tool_blocks(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            return {"status": "fail", "output": "glslangValidator missing"} if name == "shader_compiler" else result
        receipt = prerequisites.check_prerequisites(self.build, stage="development", gpu_requested=True,
                                                     probe=probe, env={})
        self.assertFalse(receipt["ok"])

    def test_python_requirement_blocks_old_interpreter(self):
        from unittest.mock import patch
        with patch.object(prerequisites.sys, "version_info", (3, 9, 9)):
            receipt = self.check()
        self.assertFalse(receipt["ok"])
        self.assertEqual("python", receipt["checks"][0]["name"])
        self.assertEqual([], self.calls)

    def test_mixed_hardware_and_cpu_inventory_blocks_hardware_claim(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            if name == "vulkan":
                result["output"] += "GPU1:\n deviceName = AMD RX\n deviceType = PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n"
            return result
        receipt = prerequisites.check_prerequisites(self.build, self.gpu_tests(), require_hardware=True,
                                                     probe=probe, env={"DISPLAY": ":99"})
        self.assertFalse(receipt["ok"])
        self.assertEqual("fail", next(row["status"] for row in receipt["checks"] if row["name"] == "hardware_gpu"))

    def test_truncated_vulkan_inventory_blocks_even_physical_candidate(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            if name == "vulkan":
                result.update(output="GPU0:\n deviceName = AMD RX\n deviceType = PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n", output_truncated=True)
            return result
        receipt = prerequisites.check_prerequisites(self.build, self.gpu_tests(), require_hardware=True,
                                                     probe=probe, env={"DISPLAY": ":99"})
        self.assertFalse(receipt["ok"])
        self.assertEqual("unknown", next(row["status"] for row in receipt["checks"] if row["name"] == "vulkan"))

    def test_hardware_development_does_not_require_display(self):
        def probe(name, command, **kwargs):
            result = self.probe(name, command, **kwargs)
            if name == "vulkan":
                result["output"] = "GPU0:\n deviceName = AMD RX\n deviceType = PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n"
            return result
        receipt = prerequisites.check_prerequisites(self.build, stage="development", require_hardware=True,
                                                     probe=probe, env={})
        self.assertTrue(receipt["ok"])
        self.assertNotIn("display", [call[0] for call in self.calls])

    def test_windows_existing_cached_compiler_path_remains_one_argument(self):
        from unittest.mock import patch
        compiler = self.build / "Program Files" / "cl.exe"
        compiler.parent.mkdir()
        compiler.touch()
        (self.build / "CMakeCache.txt").write_text("CMAKE_CXX_COMPILER:FILEPATH=" + str(compiler) + "\n")
        with patch.object(prerequisites.platform, "system", return_value="Windows"):
            receipt = self.check(stage="development")
        self.assertTrue(receipt["ok"])
        self.assertEqual(str(compiler), next(call[1][0] for call in self.calls if call[0] == "compiler_canary"))

    def test_windows_default_cl_and_flag_paths(self):
        from unittest.mock import patch
        with patch.object(prerequisites.platform, "system", return_value="Windows"):
            receipt = prerequisites.check_prerequisites(stage="development", probe=self.probe, env={})
            self.assertTrue(receipt["ok"])
            self.assertEqual("cl", receipt["compiler"])
            self.assertEqual([r'/IC:\Program Files\LLVM\include', '/MDd'],
                             prerequisites.split_arguments(r'/I"C:\Program Files\LLVM\include" /MDd'))
            receipt = prerequisites.check_prerequisites(stage="development", probe=self.probe, env={"CXX": "custom-cl"})
            self.assertEqual("custom-cl", receipt["compiler"])



class RunnerPrerequisiteIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.tests = [{"name": "gpu_numeric", "command": ["/configured/test"]}]
        self.inventory = subprocess.CompletedProcess([], 0, stdout=json.dumps({"tests": self.tests}))
        self.repo = Path(runner.__file__).resolve().parents[1]

    def test_failed_receipt_blocks_actual_ctest_suite(self):
        receipt = {"ok": False, "checks": [{"name": "sanitizer_canary", "status": "unknown"}]}
        with patch.object(runner.sys, "argv", ["run_tests.py", "--preset", "sanitize", "-R", "gpu_numeric"]), \
                patch.object(runner.subprocess, "run", return_value=self.inventory) as run, \
                patch.object(runner, "repair_permissions", return_value=[]) as repair, \
                patch.object(runner, "check_prerequisites", return_value=receipt) as check, \
                redirect_stdout(io.StringIO()) as output, redirect_stderr(io.StringIO()) as error:
            result = runner.main()
        self.assertEqual(1, result)
        self.assertEqual(1, run.call_count)
        self.assertIn("--show-only=json-v1", run.call_args.args[0])
        repair.assert_called_once_with(self.tests, self.repo / "build")
        check.assert_called_once_with(self.repo / "build/sanitize", self.tests,
                                      stage="test", require_hardware=False)
        self.assertEqual(receipt, json.loads(output.getvalue()))
        self.assertIn("blocked execution", error.getvalue())

    def test_passing_selected_inventory_reaches_suite_and_propagates_failure(self):
        command = ["ctest", "--preset", "debug", "-R", "^gpu_numeric$", "--no-tests=error"]
        with patch.object(runner.sys, "argv", ["run_tests.py", *command[1:]]), \
                patch.object(runner.subprocess, "run", side_effect=[self.inventory, subprocess.CompletedProcess(command, 7)]) as run, \
                patch.object(runner, "repair_permissions", return_value=[]), \
                patch.object(runner, "check_prerequisites", return_value={"ok": True}) as check, \
                redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            result = runner.main()
        self.assertEqual(7, result)
        self.assertEqual(command + ["--show-only=json-v1"], run.call_args_list[0].args[0])
        self.assertEqual(command, run.call_args_list[1].args[0])
        self.assertEqual(self.tests, check.call_args.args[1])
        self.assertEqual(2, run.call_count)

    def test_listing_bypasses_probes_and_repairs(self):
        for option in ("-N", "--show-only=json-v1"):
            with self.subTest(option=option), \
                    patch.object(runner.sys, "argv", ["run_tests.py", "--preset", "unconfigured-custom", option]), \
                    patch.object(runner.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as run, \
                    patch.object(runner, "repair_permissions") as repair, \
                    patch.object(runner, "check_prerequisites") as check:
                self.assertEqual(0, runner.main())
            run.assert_called_once_with(["ctest", "--preset", "unconfigured-custom", option], cwd=self.repo)
            repair.assert_not_called()
            check.assert_not_called()

    def test_custom_preset_requires_explicit_build_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            build_dir = Path(directory)
            for explicit in (False, True):
                argv = ["run_tests.py", "--preset", "local-user-preset"]
                if explicit:
                    argv += ["--prerequisite-build-dir", str(build_dir)]
                with self.subTest(explicit=explicit), \
                        patch.object(runner.sys, "argv", argv), \
                        patch.object(runner.subprocess, "run", side_effect=[self.inventory, subprocess.CompletedProcess([], 0)]) as run, \
                        patch.object(runner, "repair_permissions", return_value=[]), \
                        patch.object(runner, "check_prerequisites", return_value={"ok": True}) as check, \
                        redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()) as error:
                    result = runner.main()
                if explicit:
                    self.assertEqual(0, result)
                    self.assertEqual(2, run.call_count)
                    check.assert_called_once_with(build_dir, self.tests, stage="test", require_hardware=False)
                    self.assertNotIn("--prerequisite-build-dir", run.call_args.args[0])
                else:
                    self.assertEqual(1, result)
                    self.assertEqual(1, run.call_count)
                    check.assert_not_called()
                    self.assertIn("Custom test preset requires --prerequisite-build-dir", error.getvalue())


if __name__ == "__main__":
    unittest.main()
