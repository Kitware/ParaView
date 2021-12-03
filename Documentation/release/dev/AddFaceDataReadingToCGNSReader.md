## Add Face Data Reading To CGNS Reader

It is now possible to read either cell or face-centered data arrays in CGNS files describing meshes with 3D cells. This is done by considering the 3D cells or the 2D faces (e.g. 1 cell versus 6 faces for a cube), respectively.

Note that the element connectivity in the CGNS file must be defined with element type `NGON_n` to construct face-based meshes. Data arrays should then be defined with `GridLocation_t` either set to `CellCenter` or `FaceCenter`, respectively.

The location of the data to read can be selected using the combo box `Data Location` in the Properties tab when reading a CGNS file. The available options are `Cell Data` or `Face Data`, respectively. Note that the default value is `Cell Data`, which corresponds to the previous default behavior.
