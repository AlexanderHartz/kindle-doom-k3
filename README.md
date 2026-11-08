# kindle-doom — PrBoom for the Kindle 3

A port of [PrBoom 2.5.0](https://prboom.sourceforge.net/) to the
**Amazon Kindle 3 (Keyboard)**, launched from
[KUAL](https://www.mobileread.com/forums/showthread.php?t=203326).

This work builds on Simon Garrelou's Kindle 4 port
([`simsor/kindle-doom`](https://github.com/simsor/kindle-doom)) and turns
it into an ARMv6 / musl / static build with working sound effects, Kindle 3
keyboard support, an ALSA backend for the WM8960 codec and a tuned
800x600 framebuffer output.

**Status:** video, input and sound effects work on real Kindle 3 hardware.
Music is not implemented yet, so the launcher still passes `-nomusic`.

## Requirements

- Kindle 3 (Keyboard, ARMv6, firmware 3.x) with KUAL installed.
- An IWAD. The port looks for `doom.wad`, `doom2.wad`, `freedoom1.wad`
  or `freedoom2.wad` in `/mnt/us` or `/mnt/us/doom/`. No IWAD is bundled:
  use the Doom shareware/registered WAD you own, or
  [Freedoom](https://freedoom.github.io/).
- The bundled `prboom.wad`, which is the DFSG-cleaned Debian data file.
  See [`data/README.prboom-wad.md`](data/README.prboom-wad.md).

## Installing

1. Install KUAL on the Kindle.
2. Copy the release ZIP to the root of the Kindle USB partition and extract
   it. It creates `/mnt/us/extensions/prboom/`.
3. Copy your IWAD to `/mnt/us/doom.wad` (or `/mnt/us/doom/doom.wad`).
4. Open KUAL and run **PrBoom**.

The launcher writes diagnostics and the game log to
`/mnt/us/extensions/prboom/prboom.log`.

## Controls

The screen is rotated to 800x600 landscape, so the five-way directions are
rotated as well.

| Input | Action |
| --- | --- |
| Five-way | Move / turn |
| OK | Fire |
| Menu | Use / open doors |
| Back | Escape (game menu) |
| Keyboard `W` `A` `S` `D` | Move / turn |
| `Space` | Use |
| `Ctrl` | Fire |
| `Shift` | Run |
| `Alt` | Strafe modifier |
| `1`–`7` | Select weapon |

## Building

The build needs an **ARMv6, EABI5, soft-float** musl toolchain. The current
binary was built with an `armv6-linux-musleabi` GCC 11.2.1 cross toolchain.

```sh
export CC=/path/to/armv6-linux-musleabi-gcc
export STRIP=/path/to/armv6-linux-musleabi-strip

./scripts/build-alsa.sh   # downloads and patches alsa-lib 1.0.25
./scripts/build.sh        # builds prboom and PrBoom_KUAL.zip
```

`build-alsa.sh` applies `patches/alsa-lib-1.0.25-musl.patch`. alsa-lib 1.0.25
parses 64-bit integers with `sscanf("%Li")`, which musl misparses; without the
patch every numeric ALSA configuration value (card, device, subdevice) is read
as garbage and the audio device cannot be opened. The patch also stops the
library from redefining `struct timeval` and `struct timespec` against musl.

The resulting `PrBoom_KUAL.zip` contains the static `prboom` binary, the
launcher, `asound.conf` and the free `prboom.wad`.

## Changes from the Kindle 4 port

- ARMv6 soft-float static musl build (the original targeted ARMv7 hard-float).
- ALSA/WM8960 sound effects through a statically linked, patched alsa-lib.
- Kindle 3 keyboard and five-way input with a rotated landscape mapping.
- 800x600 4bpp framebuffer, palette-luminance LUT and configurable
  gamma/inversion (`PRBOOM_GAMMA`, `PRBOOM_INVERT`).
- KUAL launcher with a bundled ALSA configuration and diagnostic log.
- `prboom.wad` replaced by Debian's DFSG-cleaned data file.
- Helper dogs disabled (`DOGS`), matching the free WAD.
- Doom 2 / Freedoom 2 detection for the initial warp.
- Single-instance launcher: a second launch while a game is running exits
  without touching the framebuffer or the framework, preventing concurrent
  renders and duplicate framework starts.

## Known limitations

- Music is not implemented; only sound effects are mixed.
- E-ink refresh makes fast motion blurry, although the display itself is
  crisp and correctly oriented.
- Demo playback is unreliable (inherited from the original port), so the
  launcher warps directly into the first map.

## Licence and credits

The code is distributed under the **GNU GPL v2 or later**; see `COPYING`.
The bundled `prboom.wad` comes from Debian's DFSG-cleaned `prboom` package
and is also GPL-2+; see `data/README.prboom-wad.md` for provenance and the
list of removed non-free assets.

The static binary also links a patched **alsa-lib 1.0.25** (LGPL-2.1).
Its source and the patch used here are available from
`scripts/build-alsa.sh` and `patches/`.

Credits:

- id Software, for Doom and the original source release.
- The PrBoom developers.
- Simon Garrelou (`simsor`), for the Kindle 4 port this repository is based on.
- The Debian Games Team, for the DFSG-cleaned `prboom.wad`.
