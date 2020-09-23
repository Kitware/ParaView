## Fides Reader for ADIOS2 files/streams

You can now read ADIOS2 files or data streams using [Fides](https://fides.readthedocs.io/en/latest/index.html).
Fides converts the ADIOS2 data to a VTK-m dataset and the vtkFidesReader creates partitioned datasets that contain either native VTK datasets or VTK VTK-m datasets.
In order to read data and correctly map it to VTK-m data structures, Fides requires data model description written in JSON.
When using the Fides reader in ParaView, the data model is automatically generated based on some metadata stored in ADIOS attributes (see the [Fides documentation](https://fides.readthedocs.io/en/latest/components/components.html#data-model-generation) for details).
