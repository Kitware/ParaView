## `vtkCLIOptions::GetExtraArguments` now returns a reference to a vector

- Previously, `vtkCLIOptions::GetExtraArguments()` would return a copy of a
  vector of argument strings. It now returns a `const&` to a vector rather than
  forcing a copy onto callers.
