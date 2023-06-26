## Add spatial iterator to DSP plugin

A new `vtkDSPIterator` has been added to iterate over the spatial dimension
for spatio-temporal datasets. In particular, this has been designed for the
Digital Signal Processing plugin which is currently meant to work with two
types of datasets:

- Multiblock datasets where each block corresponds to a mesh point/cell.
  Each block contains a vtkTable where the tuples correspond to timesteps.
  Such a structure can be obtained by applying the filter `Plot Data Over Time`
  with the option `Only Report Selection Statistics` off.

- Table datasets containing multidimensional arrays (see `vtkMultiDimensionalArray`)
  where the rows correspond to timesteps and a third hidden dimension corresponds
  to mesh points/cells.
  This structure can be obtained by applying the filter `Temporal Multiplexing`.

The new iterator is able to traverse these two types of datasets in a transparent
way after simply giving a dataset to it as input.

A simple example of a filter using this iterator can be found in the test
`TestDSPIteratorIntegration`.
