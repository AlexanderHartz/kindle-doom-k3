# About `prboom.wad`

This file is **not** the `prboom.wad` shipped with upstream PrBoom 2.5.0.
It is the DFSG-cleaned data file from Debian's `prboom` package.

Upstream `prboom.wad` contains material that cannot be redistributed:

- helper-dog sprites and sounds taken from Wolfenstein 3D (© id Software,
  all rights reserved);
- menu graphics derived from Doom / Doom 2 (© id Software, all rights
  reserved).

The Debian Games Team removes those resources and replaces the menu
graphics with simple generated placeholders. The result is distributed
under the same GPL-2+ terms as PrBoom itself.

## Provenance

- Package: `prboom` version `2:2.5.0+dfsg2-1` (Debian)
- File: `data/prboom.wad`
- Source archive: `prboom_2.5.0+dfsg2.orig.tar.gz`
  (<https://snapshot.debian.org/package/prboom/2:2.5.0+dfsg2-1/>)
- SHA-256 of this file:
  `fb7596e0ffac932211f6eb98d5b994e40263f98d6a36d6f9f17e64ab9c0b5cea`
- Size: 166032 bytes, 124 lumps (upstream: 281020 bytes, 187 lumps)

The conversion script is in the Debian source package as
`debian/fix_upstream.sh`; the list of replaced menu graphics is in
`debian/bad_menu_lumps.txt`.

## License

GPL-2+, the same license as the rest of this repository. See `COPYING`
in the repository root.

Because this file is distributed under the GPL, the corresponding
Debian source package is available at the URL above. It contains the
DFSG-cleaning script and the original PrBoom data sources used to build
this file.
