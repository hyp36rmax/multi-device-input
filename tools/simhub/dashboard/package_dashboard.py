#!/usr/bin/env python3
"""Create a deterministic SimHub dashboard export with its required root folder."""

from pathlib import Path
import argparse
import zipfile

ROOT = Path(__file__).resolve().parent
NAME = "HYP36rforce Digital Dash"
SOURCE = ROOT / NAME
REQUIRED = (
    f"{NAME}.djson",
    f"{NAME}.djson.metadata",
)


def package(destination: Path) -> None:
    for name in REQUIRED:
        if not (SOURCE / name).is_file():
            raise SystemExit(f"missing dashboard source: {name}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for source in sorted(path for path in SOURCE.rglob("*") if path.is_file()):
            entry = (Path(NAME) / source.relative_to(SOURCE)).as_posix()
            info = zipfile.ZipInfo(entry, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, source.read_bytes())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    package(parser.parse_args().destination)
