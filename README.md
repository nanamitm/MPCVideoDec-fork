# MPCVideoDec-fork

A standalone build of **MPCVideoDec.ax**, the D3D11/DXVA2 hardware video
decoder filter from [MPC-BE](https://github.com/Aleksoid1978/MPC-BE), with a
couple of D3D11 decode fixes that didn't make it upstream. This repository
only builds and distributes that one filter - it does not build the MPC-BE
player or any of its other filters.

## Why this exists

Two bugs surfaced while running this decoder under
[BonDriver_dantto4k](https://github.com/0p1pkt2/dantto4k)-fed 4K/8K
broadcast streams in [TVTest](https://github.com/DBCTRADO/TVTest):

- **Dangling pointer crash on a 4K→8K format switch** in the DXVA2/D3D11
  hwaccel packet path (`FillAVPacket`). Reported as
  [Aleksoid1978/MPC-BE#1163](https://github.com/Aleksoid1978/MPC-BE/pull/1163);
  upstream fixed it with their own commit, and this fork's code matches that
  fix.
- **Spurious `EC_DISPLAY_CHANGED` on every output format change**, which
  makes the default DirectShow filter graph manager stop and restart the
  whole graph, occasionally leaving D3D11 device creation in a bad state
  (no video on startup or after a renderer switch). Proposed in
  [Aleksoid1978/MPC-BE#1165](https://github.com/Aleksoid1978/MPC-BE/pull/1165),
  but the maintainer wasn't convinced it was the right fix, so for now it
  only lives here.

This fork now develops and distributes its own MPCVideoDec.ax
independently, rather than continuing to open PRs upstream for it.

## Getting the decoder

Prebuilt `MPCVideoDec.ax` (x64) is published on the
[Releases](https://github.com/nanamitm/MPCVideoDec-fork/releases) page,
tagged `decoder_v*`. Register it like any other DirectShow filter
(`regsvr32 MPCVideoDec.ax`), or point a standalone-filters-aware player at
it directly.

This build registers under its own CLSIDs and filter names
(`MPC Video Decoder (nanamitm)` / `MPC Video Converter (nanamitm)`), so it
won't collide in the registry with an official MPC-BE install on the same
machine.

## Building

This repository is trimmed to just the source MPCVideoDec.ax needs (FFmpeg,
vvdec, uavs3d, soxr, speex, mfx_dispatch, DSUtil, BaseClasses, and the
filter base classes) - it's not a full MPC-BE checkout.

```
build.bat Build x64 Filters Release NoWait
```

produces `_bin\Filters_x64\MPCVideoDec.ax`. See
[`.github/workflows/decoder-release.yml`](.github/workflows/decoder-release.yml)
for the exact CI recipe (MinGW/GCC toolchain setup for the bundled FFmpeg,
then an MSVC build of the filter itself).

## License

GPLv3, same as upstream MPC-BE. See [LICENSE.txt](LICENSE.txt) and
[Aleksoid1978/MPC-BE](https://github.com/Aleksoid1978/MPC-BE) for the
original project and full credits.
