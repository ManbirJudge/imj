⚠️ **Note:** This project is work in progress.
# imj
An image library.
## Features
- Full BMP support.
- Full QOIF support.
- Full PNM support.
- Full GIF support.
  - Without animations, obviously. Only 1st frame is read.
## TODOs
- Full PNG support.
- Fix JPEG support.
- Fix includes in all files (some files have extra or deficient headers).
- Add support for following formats (in order of priority) -
  - WebP
  - ICO
  - PAM
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
### GIF
- Ability to get all frames.