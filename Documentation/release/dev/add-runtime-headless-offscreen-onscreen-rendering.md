# Add runtime selection between headless, offscreen and onscreen rendering modes

The ParaView command line executables `pvserver`, `pvpython` and `pvbatch` now support all three modes of rendering - headless, offscreen and onscreen in one build.
The rendering backend is automatically selected at runtime based upon the system capabilities such as availability of an X server or EGL drivers. You can force
a specific backend by setting an environment variable `VTK_DEFAULT_OPENGL_WINDOW` to any of these values:
1. `vtkOSOpenGLRenderWindow` for software headless rendering with OSMesa on Linux and Windows.
2. `vtkEGLRenderWindow` for hardware accelerated headless rendering on Linux.
3. `vtkXOpenGLRenderWindow` for non-headless rendering on Linux.
4. `vtkWin32OpenGLRenderWindow` for non-headless rendering on Windows.
