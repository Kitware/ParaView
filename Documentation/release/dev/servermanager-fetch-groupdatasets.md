## Replace `vtkMultiBlockDataGroupFilter` by generic `vtkGroupDataSetsFilter` as PostGatherHelper in `servermanager.Fetch`

`servermanager.Fetch` now uses generic `vtkGroupDataSetsFilter` instead of `vtkMultiBlockDataGroupFilter` as PostGatherHelper
for its reduction filter in case of composite input.\
Therefore, Fetch now works with `vtkPartitionedDataSetCollection` and `vtkPartitionedDataSet` inputs.
