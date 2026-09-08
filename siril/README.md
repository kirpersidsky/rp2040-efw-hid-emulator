# Siril VFS processing script

`VFS_Extract_HaOIII_Drizzle.py` is an experimental Siril 1.4.4+ script for
OSC sessions captured with VFS virtual filters `H` and `O`.

This first version does **not** apply dark, flat, or bias calibration.

## Install and run

1. Use Siril 1.4.4 or later.
2. Create your own writable scripts folder, for example
   `~/Documents/Siril Scripts/preprocessing/`, and copy
   `VFS_Extract_HaOIII_Drizzle.py` into it. Do not put custom files inside the
   signed Siril application bundle or its automatically updated script
   repository.
3. Add `~/Documents/Siril Scripts` under **Preferences → Scripts → Script
   Storage Directories**, refresh the list, and click **Apply**. You can also
   run `reloadscripts` in Siril's command line.
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

### Why Ha and OIII use different drizzle scales

`seqextract_HaOIII` does not produce the two channels at the same pixel
dimensions. With an RGGB/Bayer OSC frame, the extracted Ha layer uses the red
photosites and is half the source width and half the source height. The
extracted OIII layer is produced at the full source dimensions. The script
therefore uses:

```text
Ha:   seqapplyreg Ha_light   -drizzle -scale=2.0 -pixfrac=0.5 -kernel=square
OIII: seqapplyreg OIII_light -drizzle -scale=1.0 -pixfrac=1.0 -kernel=square
```

The `2.0` Ha scale restores the same output dimensions as OIII before the two
masters are aligned and combined. It is not an arbitrary enlargement applied
to the finished Ha stack. The smaller Ha `pixfrac` maps every input sample to
a smaller footprint on the output grid; the OIII path keeps a full-size
footprint at its native scale.

### What drizzle contributes

During `seqapplyreg`, Siril uses the sub-pixel offsets measured by two-pass
registration to place samples from successive frames on the output grid.
Dithered frames provide different sampling phases, so drizzle can preserve
sampling better than enlarging an already stacked image. The square kernel and
`pixfrac` determine the footprint used when each input sample is deposited.

Drizzle does **not** restore detail that was lost to poor focus, seeing,
tracking error, or undersampled data with no useful sub-pixel diversity. It
also does not replace calibration. Its main jobs in this workflow are to:

1. reconstruct the half-width/half-height Ha extraction at the dimensions used
   by the OIII layer;
2. apply registration and reconstruction as one operation;
3. retain useful sub-pixel information when the session contains adequate
   dithering and enough frames.

This asymmetric Ha/OIII processing is intentional and is why the script is a
custom VFS workflow rather than a renamed copy of a standard Siril script. The
Ha drizzle parameters themselves follow Siril's documented recommendation for
Ha/OIII extraction; the VFS-specific part is sorting frames by their virtual
`FILTER` value and focus position, processing the groups separately, then
aligning and composing their masters.

Further reading:

- [Siril: Ha/OIII extraction and output dimensions](https://siril.readthedocs.io/en/stable/processing/extraction.html)
- [Siril: drizzle scale, pixel fraction, benefits, and limitations](https://siril.readthedocs.io/en/latest/preprocessing/drizzle.html)
