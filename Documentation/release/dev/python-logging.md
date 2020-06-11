## Python Logging Integration

The `logging` Python standard library module provides an event logging system for
applications and libraries to use. ParaView (using VTK's `vtkLogger`) has its own
logging mechanism. We have now made is possible to integrate the two i.e. add an ability
to forward log messages generated in Python APIs using the `logging` standard module
to ParaView's `vtkLogger` based logging framework to be logged with other application
generated log messages.

`paraview.logger` is a standard `logging.Logger` object with handlers setup to forward messages
to `vtkLogger`. Scripts can directly access and use this `paraview.logger` or use
legacy functions like `paraview.print_error`, `paraview.print_warning`, etc. These legacy calls
simply point to the corresonding methods on the `paraview.logger` object.
