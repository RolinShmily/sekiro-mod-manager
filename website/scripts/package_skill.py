"""Build the downloadable skill using the Python standard library; no CLI binaries."""
import hashlib
import json
import zipfile
from pathlib import Path

website = Path(__file__).resolve().parents[1]
repository = website.parent
skill = repository / "skills" / "smm-cli"
output = website / "public" / "downloads"
output.mkdir(parents=True, exist_ok=True)
archive = output / "smm-cli.zip"
# Explicit allowlist keeps eval fixtures, caches, and local settings out of the archive.
files = ["SKILL.md", "references/commands.md", "scripts/run_smm.py"]
with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as package:
    for name in files:
        entry = zipfile.ZipInfo("smm-cli/" + name, date_time=(2026, 1, 1, 0, 0, 0))
        entry.compress_type = zipfile.ZIP_DEFLATED
        entry.external_attr = 0o644 << 16
        package.writestr(entry, (skill / name).read_bytes())
    license_entry = zipfile.ZipInfo("smm-cli/LICENSE.txt", date_time=(2026, 1, 1, 0, 0, 0))
    license_entry.compress_type = zipfile.ZIP_DEFLATED
    package.writestr(license_entry, (repository / "LICENSE").read_bytes())
manifest = {"name": "smm-cli", "size": archive.stat().st_size, "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(), "url": "downloads/smm-cli.zip"}
(output / "skill.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print("Packaged smm-cli.zip (%d bytes)" % manifest["size"])
