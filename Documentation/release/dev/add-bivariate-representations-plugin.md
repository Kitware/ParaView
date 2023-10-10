## Add bivariate representations plugin

This plugin brings new representations to visualize two point data arrays at the same time.

The `vtkBivariateNoiseRepresentation` uses Perlin noise to visualize the values of the 2nd array:
the noise is stronger where the values are high. Note that only scalar data can be chosen as noise array.

The `vtkBivariateTextureRepresentation` computes internally 2D textures coordinates from 2 input
arrays, allowing to use a 2D color texture as color map. Note that only scalar data can be
used for both input arrays to generate texture coordinates. Default 2D textures are shipped
with the plugin and are added to ParaView when loading it.

These representations can be used to visualize 2D and 3D surfacic data.
