# Add XArrayCFReader plugin.

This is a Python only plugin based on
xarray_support.vtkXArrayCFReader. It uses readers in XArray to read
data so it enables formats not previously available in ParaView such
as zarr and grib. Once data is in memory it is passed (using zero copy
when possible) to the vtkNetCDFCFReader to parse and create a
vtkDataSet.
