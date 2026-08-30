# AmigaOS3 classic Makefile

This archive adds `Makefile.amigaos3` for a conventional static m68k build.

## Build

clib2:

    make -f Makefile.amigaos3 clib2

Output:

    build/clib2/libSDL3_image.a

libnix:

    make -f Makefile.amigaos3 libnix

Output:

    build/libnix/libSDL3_image.a

The compiler defaults are:

    m68k-amigaos-gcc
    -mcpu=68040 -mhard-float -O2
    -std=gnu99

clib2 additionally uses:

    -mcrt=clib2 -DNOIXEMUL

libnix uses:

    -noixemul

## Enabled image formats

The initial AmigaOS3 build intentionally avoids external codec libraries.

Enabled:

- ANI
- BMP
- GIF
- JPEG through stb_image
- LBM
- PCX
- PNG through stb_image
- PNM
- QOI
- SVG through nanosvg
- TGA
- XCF
- XPM
- XV

JPEG and PNG saving are disabled in this first build.

Not enabled:

- AVIF
- JPEG XL
- TIFF
- WebP

Those can be added later with separately ported libraries if useful.

## SDL3

SDL3 headers are assumed to be installed in the compiler's normal include
path. If they are local instead:

    make -f Makefile.amigaos3 SDL3_INC=-I../SDL/include clib2

`libSDL3_image.a` is a static library. Applications link SDL3_image before
SDL3, for example:

    ... -lSDL3_image -lSDL3 ...

plus the normal AmigaOS3 SDL3 dependencies.
