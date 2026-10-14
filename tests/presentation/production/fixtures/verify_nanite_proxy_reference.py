"""Verify the checked-in reference corpus identities without NumPy."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
manifest = json.loads((HERE / "nanite_proxy_reference.json").read_text())
if manifest["text_hash_format"] != "UTF-8 with canonical LF newlines":
    raise RuntimeError("Unsupported reference text hash format")
inputs = [(HERE / "generate_nanite_proxy_reference.py", manifest["generator_sha256"]),
          (HERE / "nanite_proxy_reference.txt", manifest["oracle_sha256"])]
inputs.extend((ROOT / path, expected) for path, expected in manifest["sources"].items())
for path, expected in inputs:
    actual = hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()
    if actual != expected:
        raise RuntimeError(f"Reference identity changed: {path}; regenerate and review the corpus")
print(f"Verified {manifest['cases']} exact-source reference cases; NumPy not required")
