## New configuration option `PARAVIEW_BUILD_EDITION_STRICT`

ParaView now provides the configuration option `PARAVIEW_BUILD_EDITION_STRICT` to control the strictness of the VTK modules excluded by ParaView build edition.
By setting `PARAVIEW_BUILD_EDITION_STRICT=OFF` or configuring with `PARAVIEW_USE_EXTERNAL_VTK` any additional VTK module may be enabled for any `PARAVIEW_BUILD_EDITION`.
This feature enables building ParaView with any VTK the meets the minimum required modules for the configured build edition.
