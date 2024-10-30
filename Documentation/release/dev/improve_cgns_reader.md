## New **Load Surfaces** option in **CGNSSeriesReader**

You can now read surface (2D) elements stored in CGNS files as `Element_t`
nodes. This allows to read arbitrary surfaces saved in the file that are not
associated to a boundary condition `BC_t` node.
