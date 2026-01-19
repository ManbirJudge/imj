⚠️ **Note:** This project is work in progress.
# imj
An image library.
## Features
- Full BMP support.
- Full QOIF support.
- Full PNM support.
## TODOs
- Fix memery leaks in `png.c` and `jpeg.c`.
- Code cleanup of `jpeg.c`.
- Fix includes in all files (some files have extra or deficient headers).
- Add support for following formats (in order of priority) -
  - PNG (almost all except obscure rare cases)
  - JPEG (only common cases)
  - WebP
  - GIF (find a solution to handle animations)
  - PAM
  - ICO (almost all except obscure rare cases)
- Add MSVC compilation support.
### PNG
- PNG checks including but not limited to - chunk ordering.
- All PNG bit-depths.
- Ancillary PNG chunk support.
### BMP
- All checks.
- ~~OS/2 BMP support.~~
- Color space stuff support.
### QOIF
- `edgecase.qoi` fix.