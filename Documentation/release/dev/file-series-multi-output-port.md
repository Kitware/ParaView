## vtkFileSeriesReader supports multi-output-port readers

  `vtkFileSeriesReader` now automatically detects and matches the output port
  count of its internal reader. This enables file series support for readers
  with multiple output ports without requiring subclassing.
