# GmshReader plugin

* The XML interface file (.mshi) for the GmshReader plugin has been simplified and has become more flexible. The documentation has been updated accordingly in Plugins/GmshReader/README.md, along with the test data.
* A major Gmsh API has changed since Gmsh 4.1.0 so that another variant of the msh 2 format named SeparateViews, which is better suited for massively parallel simulations, can be handled by the plugin, along with the traditional SingleFile and SeparateFiles variants of the msh 2 format.
* The minimum Gmsh version required by the plugin has been incremented to 4.1.0.
