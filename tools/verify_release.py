"""Verify retained snapshot hashes and Keil references; no third-party packages required."""
from pathlib import Path
import hashlib
import json
import re
from xml.etree import ElementTree as ET

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "SOURCE_MANIFEST.json").read_text(encoding="utf-8"))
checked = 0
for name, variant in manifest["variants"].items():
    base = root / "firmware" / name
    if not base.exists():
        continue  # Each single-variant release archive intentionally omits the other variant.
    for relative, entry in variant["files"].items():
        path = base / relative
        assert path.is_file(), f"Missing file: {name}/{relative}"
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        assert digest == entry["release_sha256"], f"Snapshot hash differs: {name}/{relative}"
        checked += 1
    main = (base / "Core/Src/main.c").read_bytes()
    assert b'#include "wifi_config.h"' in main
    assert not re.search(rb"^\s*#define\s+(WIFI_SSID|WIFI_PASSWORD|SERVER_IP)\b", main, re.M)
    config = (base / "Core/Inc/wifi_config.example.h").read_text(encoding="utf-8")
    assert '"YOUR_WIFI_PASSWORD"' in config and '"YOUR_TCP_SERVER_IP"' in config
    project = base / "MDK-ARM/SDM18-stm32-HAL.uvprojx"
    refs = [e.text for e in ET.parse(project).findall(".//FilePath")]
    assert len(refs) == variant["keil_project_references"] == 33
    for relative in refs:
        assert (project.parent / relative.replace("\\", "/")).resolve().is_file(), relative
    for relative in ["Drivers/CMSIS/LICENSE.txt",
                     "Drivers/CMSIS/Device/ST/STM32F1xx/LICENSE.txt",
                     "Drivers/STM32F1xx_HAL_Driver/LICENSE.txt"]:
        assert (base / relative).is_file(), relative
assert checked > 0
for path in ["README.md", "README.zh-CN.md", "LICENSE", "CITATION.cff",
             "THIRD_PARTY_NOTICES.md", "docs/PROTOCOL.md", "docs/VALIDATION.md"]:
    assert (root / path).is_file(), path
print(f"PASS: {checked} retained source/template files; hashes, configuration extraction, licenses and Keil references verified.")
