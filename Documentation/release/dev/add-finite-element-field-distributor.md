## Finite Element Field Visualization I

The Finite Element Field Distributor filter (or the FEFD filter in short)
can now visualize Discontinuous Galerkin (DG) finite element fields of type H(Grad), H(Curl), and H(Div).

Note that all cells must be of the same type and the field data must
contain a `vtkStringArray` describing the DG fields, basis types, and
reference cells.
