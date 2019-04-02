# Introduce a new plugin to support for the new Explicit Structured Grid dataset

3D Explicit Structured Grid is a kind of dataset recently introduced into VTK.
It allows to represent dataset where cells are topologically structured (all
cells are hexahedrons and structured in i, j and k directions) but have explicit
definitions of their geometry. This dataset allows to represent geological or
reservoir grids for instance where faults can exist in any directions.

ParaView can now display those grids and expose a source and a set of filters
provided by VTK to generate such kind of grids, convert to/from Unstructured
Grids, efficiently slice and crop those grids.
