## ParFlow plugin

The ParFlow plugin now uses VTK's explicit structured grid[1] data format
to represent the subsurface simulation domain when terrain deflection is
turned on (i.e., when deforming the grid by elevation). This increases
memory usage but provides visualizations more useful in geology where
discontinuities are often present.

[1]: https://blog.kitware.com/introducing-explicit-structured-grids-in-vtk-and-paraview/
