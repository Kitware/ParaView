## Add Overlapping AMR specific Contour and AMR Interface filter

ParaView now provides an AMR specific Contour implementation and
an AMR Interface filter in order to provide seamless contour for
Overlapping AMR data.

AMR Interface is a filter that convert an Overlapping AMR into
a composite data of cartesian grid and unstructured grid, creating
a perfect paving of the AMR space with no discontinuities (T-cells).
It also performs data interpolation on all the new points.

Contour now properly supports Overlapping AMR input and provides
seamless contour for such input thanks to the AMR interface filter.
