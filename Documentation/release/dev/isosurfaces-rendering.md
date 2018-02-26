# Expose properties in the ImageVolumeRepresentation to render iso-contours

Thanks to a recent change in VTK, it is now possible to use the OpenGL based
volume rendering mapper to display iso-contours without computing them on the
CPU. This feature is exposed through two new properties on the image volume
representation: "Show IsoSurfaces" to enable the feature, and "Contour Values"
to specify the isosurfaces values similarely to what the "Contour" filter offers.
