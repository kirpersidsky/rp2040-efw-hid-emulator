#!/usr/bin/env python3
"""Build separate Ha and OIII stacks from VFS-tagged ASIAIR FITS lights.

This first version intentionally performs no dark, flat, or bias calibration.
Run it from Siril 1.4.4 or later. Select a home directory containing lights/.
Original files are never modified.
"""

from __future__ import annotations

import os
import shutil
import sys
from datetime import datetime
from pathlib import Path


FITS_SUFFIXES = (".fit", ".fits", ".fts")
HA_FILTERS = {"H", "HA", "H-ALPHA", "HALPHA"}
OIII_FILTERS = {"O", "OIII", "O3", "O-III"}


def read_fits_header(path: Path) -> dict[str, object]:
    """Read simple FITS header cards without loading the image data."""
    header: dict[str, object] = {}
    with path.open("rb") as stream:
        while True:
            block = stream.read(2880)
            if not block:
                raise ValueError("FITS header has no END card")
            if len(block) != 2880:
                raise ValueError("Truncated FITS header")
            for start in range(0, 2880, 80):
                card = block[start : start + 80].decode("ascii", "replace")
                keyword = card[:8].strip()
                if keyword == "END":
                    return header
                if not keyword or card[8:10] != "= ":
                    continue
                value_text = card[10:].split("/", 1)[0].strip()
                if value_text.startswith("'"):
                    value = value_text[1:].split("'", 1)[0].strip()
                elif value_text in {"T", "F"}:
                    value = value_text == "T"
                else:
                    try:
                        value = float(value_text) if any(c in value_text for c in ".Ee") else int(value_text)
                    except ValueError:
                        value = value_text
                header[keyword] = value


def classify_filter(value: object) -> str | None:
    normalized = str(value or "").strip().upper().replace(" ", "")
    if normalized in HA_FILTERS:
        return "ha"
    if normalized in OIII_FILTERS:
        return "oiii"
    return None


def discover_lights(lights_dir: Path) -> tuple[list[Path], list[Path], list[tuple[Path, object]]]:
    ha: list[Path] = []
    oiii: list[Path] = []
    unknown: list[tuple[Path, object]] = []

    files = sorted(
        path
        for path in lights_dir.rglob("*")
        if path.is_file() and path.name.lower().endswith(FITS_SUFFIXES)
    )
    for path in files:
        try:
            filter_value = read_fits_header(path).get("FILTER")
        except (OSError, ValueError) as error:
            unknown.append((path, f"unreadable: {error}"))
            continue
        group = classify_filter(filter_value)
        if group == "ha":
            ha.append(path)
        elif group == "oiii":
            oiii.append(path)
        else:
            unknown.append((path, filter_value))
    return ha, oiii, unknown


def stage_files(files: list[Path], destination: Path) -> None:
    destination.mkdir(parents=True)
    for index, source in enumerate(files, start=1):
        target = destination / f"source_{index:05d}{source.suffix.lower()}"
        try:
            os.link(source, target)
        except OSError:
            try:
                target.symlink_to(source)
            except OSError:
                shutil.copy2(source, target)


def command(siril, name: str, *arguments: object) -> None:
    rendered = " ".join([name, *(str(argument) for argument in arguments)])
    siril.log(f"VFS: {rendered}")
    siril.cmd(name, *(str(argument) for argument in arguments))


def process_channel(siril, input_dir: Path, work_dir: Path, channel: str, result_path: Path) -> None:
    work_dir.mkdir(parents=True)
    command(siril, "cd", input_dir)
    command(siril, "convert", "light", f"-out={work_dir}")
    command(siril, "cd", work_dir)
    command(siril, "seqextract_HaOIII", "light")

    if channel == "ha":
        sequence = "Ha_light"
        scale = "2.0"
        pixfrac = "0.5"
    else:
        sequence = "OIII_light"
        scale = "1.0"
        pixfrac = "1.0"

    command(siril, "register", sequence, "-2pass")
    command(
        siril,
        "seqapplyreg",
        sequence,
        "-drizzle",
        f"-scale={scale}",
        f"-pixfrac={pixfrac}",
        "-kernel=square",
    )
    command(
        siril,
        "stack",
        f"r_{sequence}",
        "rej",
        "3",
        "3",
        "-norm=addscale",
        "-output_norm",
        "-32b",
        "-out=channel_stack",
    )
    command(siril, "mirrorx_single", "channel_stack")
    command(siril, "load", "channel_stack")
    command(siril, "save", result_path.with_suffix(""))


def write_report(
    path: Path,
    home: Path,
    ha_files: list[Path],
    oiii_files: list[Path],
    unknown: list[tuple[Path, object]],
) -> None:
    lines = [
        "VFS Ha/OIII processing report",
        f"Home directory: {home}",
        f"Ha-focus frames: {len(ha_files)}",
        f"Ha integration: {total_integration(ha_files):.0f} s",
        f"OIII-focus frames: {len(oiii_files)}",
        f"OIII integration: {total_integration(oiii_files):.0f} s",
        f"Unknown or skipped frames: {len(unknown)}",
    ]
    for file, value in unknown:
        lines.append(f"  {file.name}: FILTER={value!r}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def total_integration(files: list[Path]) -> float:
    total = 0.0
    for file in files:
        value = read_fits_header(file).get("EXPTIME", 0)
        total += float(value) if isinstance(value, (int, float)) else 0.0
    return total


def verify_monochrome(path: Path) -> None:
    """Fail clearly if an intermediate/final spectral layer became RGB."""
    header = read_fits_header(path)
    if header.get("NAXIS") != 2:
        raise RuntimeError(
            f"Expected a monochrome FITS image, but {path.name} has "
            f"NAXIS={header.get('NAXIS')} and NAXIS3={header.get('NAXIS3')}"
        )


def run() -> None:
    import sirilpy as s

    s.ensure_installed("PyQt6")
    from PyQt6.QtWidgets import QApplication, QFileDialog, QMessageBox

    siril = s.SirilInterface()
    siril.connect()
    app = QApplication.instance() or QApplication(sys.argv)

    selected = QFileDialog.getExistingDirectory(None, "Select VFS session home directory")
    if not selected:
        return

    home = Path(selected).expanduser().resolve()
    lights_dir = home / "lights"
    if not lights_dir.is_dir():
        QMessageBox.critical(None, "VFS processing", f"The selected directory has no lights folder:\n{lights_dir}")
        return

    ha_files, oiii_files, unknown = discover_lights(lights_dir)
    if unknown:
        details = "\n".join(f"{path.name}: FILTER={value!r}" for path, value in unknown[:12])
        QMessageBox.critical(
            None,
            "VFS processing",
            "Some FITS files could not be classified. Nothing was processed.\n\n" + details,
        )
        return
    if len(ha_files) < 2 or len(oiii_files) < 2:
        QMessageBox.critical(
            None,
            "VFS processing",
            f"At least two frames are required for each focus group.\nHa: {len(ha_files)}\nOIII: {len(oiii_files)}",
        )
        return

    run_id = datetime.now().strftime("%Y%m%d-%H%M%S")
    ha_seconds = int(round(total_integration(ha_files)))
    oiii_seconds = int(round(total_integration(oiii_files)))
    ha_result_name = f"result_Ha_{ha_seconds}s"
    oiii_result_name = f"result_OIII_{oiii_seconds}s"
    hoo_result_name = f"result_HOO_Ha{ha_seconds}s_OIII{oiii_seconds}s"
    process_root = home / "vfs_process" / run_id
    result_dir = home / "results" / run_id
    ha_input = process_root / "ha_input"
    oiii_input = process_root / "oiii_input"
    result_dir.mkdir(parents=True)
    stage_files(ha_files, ha_input)
    stage_files(oiii_files, oiii_input)

    try:
        siril.update_progress("Preparing VFS Ha stack", 0.05)
        process_channel(siril, ha_input, process_root / "ha", "ha", result_dir / "results_00001.fit")
        siril.update_progress("Preparing VFS OIII stack", 0.45)
        process_channel(siril, oiii_input, process_root / "oiii", "oiii", result_dir / "results_00002.fit")

        siril.update_progress("Aligning and normalizing result stacks", 0.85)
        command(siril, "cd", result_dir)
        command(siril, "setref", "results", "1")
        command(siril, "register", "results", "-transf=shift", "-interp=none")
        expression = (
            "$r_results_00002$*mad($r_results_00001$)/mad($r_results_00002$)"
            "-mad($r_results_00001$)/mad($r_results_00002$)*median($r_results_00002$)"
            "+median($r_results_00001$)"
        )
        command(siril, "pm", f'"{expression}"')
        command(siril, "save", oiii_result_name)
        command(siril, "load", "r_results_00001")
        command(siril, "save", ha_result_name)

        # The two spectral masters must remain single-channel. rgbcomp then
        # creates the only intentional three-channel result: R=Ha, G/B=OIII.
        verify_monochrome(result_dir / f"{ha_result_name}.fit")
        verify_monochrome(result_dir / f"{oiii_result_name}.fit")
        command(
            siril,
            "rgbcomp",
            ha_result_name,
            oiii_result_name,
            oiii_result_name,
            f"-out={hoo_result_name}",
            "-nosum",
        )
        write_report(result_dir / "processing_report.txt", home, ha_files, oiii_files, unknown)
        command(siril, "load", hoo_result_name)
        siril.update_progress("VFS processing complete", 1.0)
        QMessageBox.information(
            None,
            "VFS processing complete",
            f"Created monochrome Ha and OIII masters plus an RGB HOO image:\n"
            f"{ha_result_name}.fit\n{oiii_result_name}.fit\n{hoo_result_name}.fit\n\n"
            f"Saved in:\n{result_dir}",
        )
    except Exception as error:
        siril.log(f"VFS processing failed: {error}")
        QMessageBox.critical(
            None,
            "VFS processing failed",
            f"{error}\n\nIntermediate files were kept in:\n{process_root}",
        )
        raise
    finally:
        siril.reset_progress()


if __name__ == "__main__":
    run()
