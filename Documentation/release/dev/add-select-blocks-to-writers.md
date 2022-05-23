## Add block selection to writers

Add a new option in the `Configure Writer` dialog for the `XMLMultiBlockDataWriter`, `XMLPartitionedDataSetCollectionWriter`and `XMLUniformGridAMRWriter` allowing to choose blocks to write.
The `vtkSelectArraysExtractBlocks` meta-filter is added for this purpose.
It combines `vtkPassSelectedArrays` and `vtkExtractBlockUsingDataAssembly` and acts as a pre-processing filter for the concerned composite writers.

You can add this pre-processing filter in a writer by including the proxy `SelectArraysExtractBlocks` as a subproxy in the writer proxy definition
(see the `FileSeriesWriterComposite` proxy definition in `Remoting/Application/Resources/internal_writers.xml` for an example).
