PVLocal is a sample plugin for ParaView.  It is intended to be copied
by users to help them start their own ParaView plugin.  Here are the
steps:

1.)  Copy this source tree to another location outside of ParaView.

2.)  Replace vtkLocalConeSource with one or more of your own VTK
     classes.  Be sure to include vtk(project-name)Configure.h
     and use the VTK_(project-name)_EXPORT macro for class declarations.

3.)  Edit CMakeLists.txt and set the project name, PVLocal_SRCS,
     and PVLocal_LIBS variables for your classes.

4.)  Edit PVLocal.xml.in and replace the interface specification
     with those for your classes.

5.)  Build the project outside of ParaView.

6.)  Import the package into ParaView.
