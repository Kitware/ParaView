# Glyph3D quaternion selection

When Glyph3D representation is selected, one can use a data array to define the orientation of the glyphs.
So far, one had choice between direction vector or angles around three axes (both by selecting a 3-components array).
We added a new "Quaternion" mode to specify the orientation through a 4-components point array with quaternions defined in WXYZ format.
