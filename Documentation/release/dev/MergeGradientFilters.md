## The filters Gradient and Gradient Of Unstructured DataSet have been merged

Before ParaView 5.10, these two filters could be used to compute the gradient of a dataset:
- `Gradient` based on `vtkImageGradient` for `vtkImageData` only
- `Gradient Of Unstructured DataSet` based on `vtkGradientFilter` for all `vtkDataSet`

These filters have been replaced with a unique `Gradient` filter based on `vtkGradientFilter` and including the functionalities from the former `Gradient` filter. The default behavior always uses the `vtkGradientFilter` implementation.

For `vtkImageData` objects, it is still possible to use the `vtkImageGradient` implementation through the `BoundaryMethod` property, which has two options defining the gradient computation at the boundaries:
- `Smoothed` corresponds to the `vtkImageGradient` implementation (the old `Gradient`) and uses central differencing for the boundary elements by duplicating their values
- `Non-Smoothed` corresponds to the `vtkGradientFilter` implementation (the old `Gradient Of Unstructured DataSet`) and simply uses forward/backward differencing for the boundary elements

For all other `vtkDataSet` objects, the filter usage is unchanged.
