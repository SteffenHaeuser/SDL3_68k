# AmigaOS 68k classical build

Use `make -f Makefile.amiga` to build both clib2 and libnix variants, including the shipped test and example programs.

Individual variants:

```sh
make -f Makefile.amiga clib2
make -f Makefile.amiga libnix
```

Outputs:

- clib2: `build/clib2/lib/libSDL3_sound.a`
- libnix: `build/libnix/lib/libSDL3_sound_libnix.a`
- tests/examples are under the matching `build/.../bin/` directory.

The build disables GCC builtins for `floor`, `ceil`, `sin`, `cos`, and `log10`.

Enabled decoders are the upstream self-contained defaults: WAV, AIFF, AU, VOC, FLAC (dr_flac), Vorbis (stb_vorbis), RAW, SHN, ModPlug, and MP3 (dr_mp3). MIDI/TiMidity remains disabled, as in the upstream default.
