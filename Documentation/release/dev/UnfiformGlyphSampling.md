# Glyph filter new uniform inverse transform sampling mode
This adds two new uniform glyphing modes to the vtkPVGlyphFilter.
The two new modes are based on "inverse transform sampling" algorithm, the first one uses surface area, the second one uses cell volume.

The surface one will actually extract the surface first if needed, so the glyphs will be located on the surface mesh only.
The volume one will ignore completely non-3D cells.

Both modes support multiblocks and parallel meshes perfectly, as well as cell centers when using cell data.
