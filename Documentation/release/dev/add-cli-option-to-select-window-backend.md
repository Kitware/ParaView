# Add option to select OpenGL window backend

You can now use the new `--opengl-window-backend` command line option to specify
the OpenGL window backend used in `pvpython`, `pvbatch`, `pvserver` and the `pvrenderserver` executables.
Supported values are 'GLX', 'EGL', 'OSMesa', 'Win32'. It is not applicable to the ParaView client.
If not specified, the default backend is used. The default backend is determined by the build configuration and the
hardware configuration the machine.
