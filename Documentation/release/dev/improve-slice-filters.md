## Improve slice filters

Slice filters have the following improvements:

1. `Slice With Plane` now supports `vtkHyperTreeGrid` and `vtkOverlappingAMR`.
2. `AMR CutPlane` has been deprecated, because it has been integrated inside the `Slice With Plane` filter.
   Use the `Slice With Plane` filter with Plane type = `Plane` to achieve the same functionality.
3. `Slice AMR data` has been deprecated, because it has been integrated inside the `Slice With Plane` filter.
   Use the `Slice With Plane` filter with Plane type = `Axis Aligned Plane` to achieve the same functionality.
4. `Slice With Plane`'s performance has been significantly improved for `vtkStructuredGrid`, `vtkRectilinearGrid`,
   `vtkUnstructuredGrid` with 3d linear cells, and `vtkPolyData` with convex cells.
5. `Slice with Plane` now supports the generation of polygons for `vtkImageData`.
6. `Slice With Plane` now has a MergePoints flag to specify if output points will be merged.
7. `Slice` performance has been significantly improved because it delegates to `Slice With Plane`, whenever possible.
   1. `Slice` can't delegate to `Slice With Plane` for `vtkOverlappingAMR`, because it's not composite-dataset aware.
