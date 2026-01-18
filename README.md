⚠️ **Note:** This project is work in progress.
# imj
An image library.

## TODOs
- PNG checks including but not limited to - chunk ordering.
- All PNG bit-depths.
- Ancillary PNG chunk support.
- Fix memery leaks in `png.c` and `jpeg.c`.
- Code cleanup of `jpeg.c`.
- Fix includes in all files (some files have extra or deficient headers).
- QOIF `edgecase.qoi` fix.
- Add support for following formats (in order of priority) -
  - PNG (almost all except obscure rare cases)
  - JPEG (only common cases)
  - BMP (all)
  - WebP
  - GIF (find a solution to handle animations)
  - PNM and PAM (all)
  - QOI (all)
  - ICO (almost all except obscure rare cases)