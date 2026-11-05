# Add distributed abort support

It is now possible to abort during filter execution while they are being executed,
even in distributed client/server mode. However, not all filters and source supports that.

Supported by:
 - Calculator
 - Cell Data to Point Data
 - Clip
 - Extract Block
 - Glyph
 - Group Datasets
 - Slice
 - Stream Tracer
 - Threshold
 - Warp By Scalar
 - Warp By Vector
 - Wavelet
