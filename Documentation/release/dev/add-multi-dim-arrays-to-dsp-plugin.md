## Add multi-dimensional arrays to DSP plugin

Add `vtkMultiDimensionalArray` to the DSP plugin. For now, this implicit array allows to represent 3D arrays and provides "2D views" on it.
The motivation is to use in the DSP plugin in order to avoid relying on multiblock datasets to represent temporal data at each point of a mesh
(output of the "Plot Data Over Time" filter without the option "Only Report Selection Statistics"), that are not scalable in a distributed environment.
An example of generation of `vtkMultiDimensionalArray` from temporal data can be found in `TestTemporalDataToMultiDimensionalArray.cxx`.
