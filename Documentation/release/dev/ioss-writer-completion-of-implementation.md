## IOSSWriter: Completion of Implementation

`IOSSWriter` is a writer for the Exodus file format that has now been completely implemented.
`IOSSWriter` is implemented using the [IOSS library](https://sandialabs.github.io/seacas-docs).
The input can be a `vtkPartitionedDataSetCollection`, `vtkPartitionedDataSet`, or `vtkDataSet`.
It supports the following entity types: Node/Edge/Face/Element blocks, and Node/Edge/Face/Element/Side sets. If the
input of the writer originated from `IOSSReader`, the entity types can be automatically deduced. Otherwise,
`AssemblyName` and `Selectors` can be used to specify which block belong in which entity type. If no selector has
been defined, and the input does not originate from IOSS, then all blocks will be treated as Element Blocks. The arrays
(or fields) to be written for each entity type can be specified after `ChooseArraysToWrite` has been set
to true. A subset of the timesteps to be written can be selected using `TimeStepRange` and `TimeStepStride`. Ghost
cells can be removed using `RemoveGhosts`. If the input has been transformed, e.g. using Clip, the original ids of
blocks can be preserved using `PreserveOriginalIds`. The Quality assurance and information records can be written
using `WriteQAAndInformationRecords`. `Global IDs` are created assuming uniqueness for both Node Blocks, i.e. points,
and Edge/Face/Element blocks, if they are not present, or invalid. `element_side` is needed for Edge/Face/Element/Side
sets. If it's not present, sets will be skipped. If it's present, but invalid, and there were (old) `Global IDs` that
were invalid, then they will be used to re-create `element_side`, otherwise, sets will be skipped.
Finally, `IOSSWriter` is now parallel-aware, therefore, it can be used in parallel.
