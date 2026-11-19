# Add distributed abort support

It is now possible to abort during filter execution while they are being executed,
even in distributed client/server mode. However, not all filters and source supports that.

Supported by:
 - Calculator
 - Cell Data to Point Data
 - Clip
 - Contour
 - Extract Block
 - Flying Edges 2D (provided by AcceleratedAlgorithms plugin)
 - Flying Edges 3D (provided by AcceleratedAlgorithms plugin)
 - Glyph
 - Group Datasets
 - Resample To Image
 - Slice
 - Stream Tracer
 - Threshold
 - Warp By Scalar
 - Warp By Vector
 - Wavelet
