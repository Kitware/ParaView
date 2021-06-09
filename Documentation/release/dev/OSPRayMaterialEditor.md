# OSPRay Material Editor

A new OSPRay material editor widget has been added to control the OSPRay materials by enabling it under the "View... Material Editor" menu. You need to compile ParaView with `PARAVIEW_ENABLE_RAYTRACING` cmake option set to ON to enable the editor widget.

This editor allows to :

* Load a material library using a .json file or create materials directly in the GUI
* Edit / Add / Remove properties on the materials
* Attach the selected material to the current selected source
