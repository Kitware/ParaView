## Standard Paths

The new `vtkPVStandardPaths` groups methods to get the standard paths
an application may want to use. This includes system directories, install
directories and user directories. They of course depends on the operating system.

On linux and MacOs, it tries to match the [XDG specifications](https://specifications.freedesktop.org/basedir-spec/0.8).

Some environment variables may modify the behavior: those defined by XDG when relevant.
On windows `APPDATA` is used to get the User directory.
