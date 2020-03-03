## Adaptive resample to image

ParaView now supports ability to adaptively resample to a collection of
vtkImageData using the **Adaptive Resample to Image** filter. The filter builds
a kd-tree approximately load balancing the input data points across requested
number of blocks (or ranks) and the resamples the region defined by each of the
kd-tree leaves to a vtkImageData.
