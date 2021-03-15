## Changes to vtkPVDataInformation

vtkPVDataInformation implementation has been greatly simplified.

vtkPVDataInformation no longer builds a complete composite data hierarchy
information. Thus, vtkPVCompositeDataInformation is no longer populated
and hence removed. This simplifies the logic in vtkPVDataInformation
considerably.

vtkPVDataInformation now provides assess to vtkDataAssembly
representing the hierarchy for composite datasets. This can be used by
application to support cases where information about the composite
data hierarchy is needed. For vtkPartitionedDataSetCollection, which can
other assemblies associated with it, vtkPVDataInformation also
collects those.

For composite datasets, vtkPVDataInformation now gathers information
about all unique leaf datatypes. This is useful for applications to
determine exactly what type of datasets a composite dataset is comprised
of.

vtkPVTemporalDataInformation is now simply a subclass of
vtkPVDataInformation. This is possible since vtkPVDataInformation no
longer includes vtkPVCompositeDataInformation.
