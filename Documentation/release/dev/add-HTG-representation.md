## Added a new HyperTreeGridRepresentation using the vtkOpenGLHyperTreeGridMapper

This representation behave like the GeometryRepresentation by default and add 3 new modes for 2D HTG:
* HTG Surface
* HTG Surface With Edges
* HTG Wireframe
When using these mode, if the Camera Parallel Projection is set to ON (and the HTG is 2Dimensional),
then we can activate the AdaptiveDecimation to only map the part of the HTG visible on the screen.
This should result in better rendering performance.
