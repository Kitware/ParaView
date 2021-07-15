## Added support for empty Mesh Blueprint coming from Conduit to Catalyst

Catalyst 2.0 used to fail when the simulation code sends a mesh through
Conduit with a full Conduit tree that matches the Mesh Blueprint but
without any cells or points. This can for instance happen on
distributed data. This commit checks the number of cells in the Conduit
tree and returns an empty vtkUnstructuredGrid when needed. This commit
only fixes the issue for unstructured meshes. This commit also fixes a
crash on meshes containing a single cell (rare occasion, but can
happen).

See https://gitlab.kitware.com/paraview/paraview/-/issues/20833
