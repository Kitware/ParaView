## Fix the behavior of vtkCLIOptions::SetStopOnUnrecognizedArgument

SetStopOnUnrecognizedArgument was behaving in an inverted way according to its name
and documentation. This was fixed and documentation was clarified.

Now, `vtkCLIOptions::SetStopOnUnrecognizedArgument(true)` will indeed stop the parsing
on unrecognized arguments.

See https://github.com/CLIUtils/CLI11/issues/1052 for more info.
