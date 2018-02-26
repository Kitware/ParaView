# Glyphs Representation Culling

It is now possible to enable Glyphs representation culling and level of details.
This allows to render a higher number of glyphs at interactive rate.
When the option is enabled, instances that are not visible are discarded and not drawn.
One can also specify LODs parameters inside a table, the first column is the distance after which
the LOD is enabled, the second column is the reduction factor applied to the instance geometry.
It is also possible to display instances in different colors depending on the LOD.
Please note that this feature works only with OpenGL driver >= 4.0.
