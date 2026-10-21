"""Offline checks for the configured macOS command-line launcher."""

import os
from pathlib import Path
import shlex
import subprocess

import pytest


@pytest.mark.parametrize("explicit", [True, False])
def test_launcher_preserves_spaced_paths_and_arguments(tmp_path, explicit):
    tools = tmp_path / "Launch Tools"
    tools.mkdir()
    app_dir = tmp_path / "Application Collection" if explicit else tools
    executable = app_dir / "ITK-SNAP.app/Contents/MacOS/synthetic-test-cli"
    executable.parent.mkdir(parents=True)
    executable.write_text("#!/bin/sh\nprintf '%s\\n' \"$@\"\n")
    executable.chmod(0o755)

    source = Path(__file__).resolve().parents[2]
    template = source / "Utilities/MacOS/BundleResources/execaller.sh"
    text = template.read_text().replace("@EXENAME@", "MacOS/synthetic-test-cli")
    text = text.replace("@EXEOPTS@", "--synthetic-option")
    # Keep fixed discovery roots away from installed applications.
    unused = shlex.quote(str(tmp_path / "Unused Applications"))
    text = text.replace("~/Applications", unused)
    text = text.replace('"$HOME/Applications"', unused)
    text = text.replace(" /Applications; do", f" {unused}; do")
    launcher = tools / "synthetic-launcher"
    launcher.write_text(text)
    environment = os.environ.copy()
    environment["ITKSNAP_APP_DIR"] = str(app_dir) if explicit else ""
    result = subprocess.run(
        ["/bin/sh", str(launcher), "argument with spaces", "literal;argument"],
        env=environment, capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stderr or result.stdout
    assert result.stdout.splitlines() == [
        "--synthetic-option", "argument with spaces", "literal;argument"
    ]
