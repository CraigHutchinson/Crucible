"""Verify that restored test artifacts are repaired without widening chmod scope."""
import os
from pathlib import Path
import stat
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from run_tests import repair_permissions


@unittest.skipUnless(os.name == "posix", "Unix artifact permissions")
class PermissionRepairTests(unittest.TestCase):
    def test_only_configured_owned_native_artifacts_are_repaired(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve() / "build"
            root.mkdir()
            target = root / "test"
            other = root / "unselected"
            script = root / "script.py"
            for path in (target, other):
                path.write_bytes(b"\x7fELFfixture")
                path.chmod(0o640)
            script.write_text("print('test')\n")
            script.chmod(0o644)
            inventory = [{"command": [str(target), "--fixture"]},
                         {"command": [str(target)]}, {"command": [str(script)]}]
            self.assertEqual(repair_permissions(inventory, root), [target])
            self.assertEqual(stat.S_IMODE(target.stat().st_mode), 0o750)
            self.assertEqual(stat.S_IMODE(other.stat().st_mode), 0o640)
            self.assertEqual(stat.S_IMODE(script.stat().st_mode), 0o644)
            self.assertEqual(repair_permissions(inventory, root), [])

    def test_declared_native_child_is_repaired_without_scanning_arguments(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve() / "build"
            root.mkdir()
            child, unregistered = root / "child", root / "argument"
            for path in (child, unregistered):
                path.write_bytes(b"\x7fELFfixture")
                path.chmod(0o640)
            inventory = [{"command": [sys.executable, "harness.py", str(child), str(unregistered)],
                          "properties": [{"name": "REQUIRED_FILES", "value": [str(child)]}]}]
            self.assertEqual(repair_permissions(inventory, root), [child])
            self.assertEqual(stat.S_IMODE(child.stat().st_mode), 0o750)
            self.assertEqual(stat.S_IMODE(unregistered.stat().st_mode), 0o640)
            child.chmod(0o640)
            inventory[0]["properties"][0]["value"] = str(child)
            self.assertEqual(repair_permissions(inventory, root), [])

    def test_macho_test_artifacts_are_repaired(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            target = root / "test"
            target.write_bytes(bytes.fromhex("cffaedfe") + b"fixture")
            target.chmod(0o644)
            self.assertEqual(repair_permissions([{"command": [str(target)]}], root), [target])
            self.assertEqual(stat.S_IMODE(target.stat().st_mode), 0o755)

    def test_missing_commands_remain_for_ctest_to_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            self.assertEqual(repair_permissions([{"command": [str(root / "missing")]}], root), [])

    def test_external_commands_and_symlinks_keep_their_modes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve() / "build"
            root.mkdir()
            external = Path(directory).resolve() / "outside"
            external.write_bytes(b"\x7fELFfixture")
            external.chmod(0o644)
            link = root / "linked"
            link.symlink_to(external)
            alias = Path(directory).resolve() / "alias"
            alias.symlink_to(root, target_is_directory=True)
            inside = root / "inside"
            inside.write_bytes(b"\x7fELFfixture")
            inside.chmod(0o644)
            inventory = [{"command": [str(external)]}, {"command": [str(link)]},
                         {"command": []}]
            self.assertEqual(repair_permissions(inventory, root), [])
            self.assertEqual(stat.S_IMODE(external.stat().st_mode), 0o644)
            self.assertEqual(repair_permissions([{"command": [str(alias / "inside")]}], root), [inside])


if __name__ == "__main__":
    unittest.main()
