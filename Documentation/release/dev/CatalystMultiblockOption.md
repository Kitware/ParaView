# Allows vtkConduitSource to output MultiBlock

The `vtkConduitSource` can optionally convert its output to a `vtkMultiBlockDataSet`.

ParaView Conduit Blueprint protocol now supports an optional `state/multiblock`
integer node inside the global or channel nodes. When existing and set
to a integer other than 0, the vtkConduitSource is setup to output multiblock.
