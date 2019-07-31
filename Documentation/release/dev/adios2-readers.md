# New support for ADIOS 2.x readers

ParaView now has support for two different readers using ADIOS 2:

* ADIOS2CoreImageReader: Read ADIOS BP files with 2D and 3D image data by
explicity specifying the various elements to populate metadata from.

* ADIOS2VTXReader: Read ADIOS BP files which contain metadata in either an
attribute or as a file in the .bp directory describing the metadata in
VTK XML format.  Support vtkImageData and vtkUnstructuredGrid.
