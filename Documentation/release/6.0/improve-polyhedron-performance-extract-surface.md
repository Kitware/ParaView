## Extract Surface and Surface Representation: Improve performance for polyhedron

`Extract Surface` and `Surface Representation` (which uses `Extract Surface` internally) used to extract the whole cell
when a cell was a polyhedron. Now, it accesses the `GetPolyhedronFaces` and `GetPolyhedronFaceLocations` methods
to efficiently extract the polyhedron faces related information.
