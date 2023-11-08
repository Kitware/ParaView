## Add ERF Reader

You can now read ERF HDF5 file into ParaView. It is based on the version 1.2 of the ERF HDF5 spec:

https://myesi.esi-group.com/ERF-HDF5/doc/ERF_HDF5_Specs_1.2.pdf

It supports:

* Reading a selected stage.
* Reading the 'constant' group.
* Reading the 'singlestate' group.

The output of the reader is a `vtkPartitionedDataSetCollection` composed of multiple unstructured grid for the 'constant' or 'singlestate' group.
