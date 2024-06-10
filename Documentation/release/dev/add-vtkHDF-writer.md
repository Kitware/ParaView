## Expose vtkHDF writer

Expose the vtkHDF writer as a writer and as an extractor in ParaView.

Currently, this writer supports writing Unstructured Grid
and PolyData datasets, possibly transient, as well as
composite types Partitioned Dataset Collection
and Multiblock Dataset, without transient support.

This writer can also write compressed data to save storage space.

The exporter writes time-dependent data as file series,
not using the transient mechanism of vtkHDF.
