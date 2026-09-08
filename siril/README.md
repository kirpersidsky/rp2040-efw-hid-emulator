# Siril VFS processing script

`VFS_Extract_HaOIII_Drizzle.py` is an experimental Siril 1.4.4+ script for
OSC sessions captured with VFS virtual filters `H` and `O`.

This first version does **not** apply dark, flat, or bias calibration.

## Install and run

1. Use Siril 1.4.4 or later.
2. Copy `VFS_Extract_HaOIII_Drizzle.py` into a folder listed in Siril's script
   paths. On macOS, the default user folder is usually
   `~/Library/Application Support/org.siril.Siril/siril-scripts/`.
3. Refresh the script list or restart Siril.
4. Open **Scripts** and run **VFS Extract HaOIII Drizzle**.
5. Choose the parent session folder—the folder containing `lights/`, not the
   `lights/` folder itself.

The first run may ask Siril for permission to install the Python GUI dependency
used by the folder picker. The original FITS files are never changed.

## Input

Choose a home directory containing:

```text
session/
└── lights/
    ├── ... FILTER='H' FITS files
    └── ... FILTER='O' FITS files
```

The script reads the original FITS `FILTER` cards, stages the two focus groups
without modifying the originals, extracts Ha/OIII, performs two-pass
registration and drizzle, stacks both channels, aligns the two stacks, and
normalizes OIII to Ha.
It also creates a ready-to-process HOO composition with Ha in red and OIII
in both green and blue.

## Output

Each run creates a timestamped directory:

```text
session/results/YYYYMMDD-HHMMSS/
├── result_Ha_9000s.fit
├── result_OIII_9000s.fit
├── result_HOO_Ha9000s_OIII9000s.fit
├── results_00001.fit
├── results_00002.fit
└── processing_report.txt
```

The number in each final filename is the total exposure time of the frames
used for that focus group. Ha and OIII are saved as separate monochrome
spectral layers. The `result_HOO_...` file is the intentional three-channel
image (`R=Ha`, `G=OIII`, `B=OIII`). `-nosum` is used when composing it so its
FITS exposure metadata is not incorrectly doubled by reusing OIII twice.

Intermediate files are retained under `session/vfs_process/` for inspection.

## Drizzle settings

- Ha: scale 2.0, pixfrac 0.5, square kernel
- OIII: scale 1.0, pixfrac 1.0, square kernel

The two final stacks are shift-aligned with Ha as the reference image.
