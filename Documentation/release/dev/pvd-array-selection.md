# Selecting arrays to read in PVD reader

PVD reader (`vtkPVDReader`) now supports selecting which arrays to read.
This can be useful to minimize memory foot print and reduce I/O times when reading PVD files
by skipping arrays that may not be needed.
