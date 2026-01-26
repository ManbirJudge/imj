⚠️ **Note:** This project is work in progress.
# imj
An image library.
## Features
- Full PNG support.
  - Without Adam7 interlacing.
  - Without CRC checks.
- Full BMP support.
  - Without embedded PNGs and JPEGs.
- Full GIF support.
  - Without animations. Only 1st frame is read.
- Full QOIF support.
- Full PNM support.
## TODOs
- Use `goto` for error handling.
- JPEG support.
  - ~~Baseline DCT~~
  - Progressive DCT
  - Understand the spec. (I had implemented a LONG time ago and forgot everything.)
  - Reset marker
- Fix includes in all files (some files have extra or deficient headers).
- Add support for following formats (in order of priority) -
  - WebP
  - ICO
  - PAM
- Add MSVC compilation support.
- Add Linux compilation support.
### PNG
- ~~All PNG bit-depths.~~
- CRC check.
- Fix memory leaks (if any).
- Adam7 interlacing.
- ~~All bit-depths.~~
  - ~~Generalized filtered stream reader.~~
    - ~~Also handle 16-bit case.~~
- ~~Proper transparency and alpha handling.~~
  - ~~Fix broken alpha channel in 32-bit RGBA PNGs.~~
  - ~~`tRNS` chunk.~~
### BMP
- ~~OS/2 BMP support.~~
### QOIF
- `edgecase.qoi` fix.
### GIF
- Ability to get all frames.
## Exotic Image Formats to Learn About
- TIFF
- PCX
- ILBM
- TGA
- DDS
- SGI RGB
- OpenEXR
- YCbCr Raw
- YUV4MPEG
- CIE Lab TIFF
- Farbfeld
- PIC
- Radiance HDR
- JPEG variants -
  - JPEG LS
  - JPEG 200
- HEIF/AVIF