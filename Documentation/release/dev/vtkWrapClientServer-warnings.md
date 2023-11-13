## vtkWrapClientServer-warnings

The `vtk_module_wrap_client_server` CMake API now supports adding warning flags
when wrapping. Currently supported is `-Wempty` to warn when a wrapping
generates no usable code.
