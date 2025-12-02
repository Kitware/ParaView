## USD scene exporter

A USD scene exporter is now available in ParaView. It exports the following:

* Surface polygonal geometry including point normals if available, cell normals otherwise.
* Color mapped surfaces through export of color map textures and texture coordinates on surfaces.
* Directional lights.
* The current camera, including position and direction. Field of view is hard-coded for now.
* Actor transforms.
* Solid colored surfaces, including their color and physically-based rendering (PBR) properties supported by the USD format when PBR is enabled.
* All surfaces from composite poly data.

The USD scene exporter does not export:

* Surfaces displayed in these representations: Feature Edges, Outline, Point Gaussian, Points, Surface LIC, or Wireframe. Edges in the Surface with Edges representation are also not exported. The USD format does not support vertex or line cells.
* Volume representations.
* 3D widgets that are subclasses of `vtkWidgetRepresentation`. These are usually temporary widgets and not important parts of a scene.
* Color legends.
* Grid or Data Axes.
* Annotations that are not part of the 3D scene, e.g. any of the annotation sources or filters or the **Text** source.
