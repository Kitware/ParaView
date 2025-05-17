# Fix in Embossing Representation plugin: Shader undeclared uniform and invalid OpenGL enum

When using embossing representation plugin, some errors occurred on the OpenGL level.

1. Shader generation fix: ParaView used to throw an error message when extrusion surface was used without lighting on (e.g. disabling light kit). This error message is now prevented.
2. OpenGL error fix: when extrusion surface or bumpmap was used with vtkIdType data, an error used to show up because the type was unknown. ParaView still does not support vtkIdType data in this plugin, but it is now properly handled, preventing low level errors from OpenGL.
