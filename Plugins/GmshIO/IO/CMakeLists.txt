set(classes
  vtkGmshReader
  vtkGmshWriter
)

vtk_module_find_package(
  PACKAGE Gmsh
  VERSION 4.7.0)

vtk_module_add_module(GmshIO::vtkGmshIO
  CLASSES ${classes})
vtk_module_link(GmshIO::vtkGmshIO
  PRIVATE
    Gmsh::Gmsh)

paraview_add_server_manager_xmls(
  XMLS  GmshIO.xml)
