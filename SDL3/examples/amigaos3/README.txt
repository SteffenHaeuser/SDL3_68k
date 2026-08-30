SDL3 AmigaOS3 smoke tests
=========================

Software renderer test:
  make -f Makefile.amigaos3 examples-clib2
  -> build/clib2/examples/test_sw

MiniGL/OpenGL test:
  same command also builds:
  -> build/clib2/examples/test_gl

For libnix:
  make -f Makefile.amigaos3 examples-libnix
  -> build/libnix/examples/test_sw
  -> build/libnix/examples/test_gl

Both runtimes:
  make -f Makefile.amigaos3 examples-both

Both tests need no external data files. Press Escape or close the window to exit.
The software test explicitly requests SDL's "software" renderer.
The GL test creates an SDL_WINDOW_OPENGL window and uses minigl.library.
