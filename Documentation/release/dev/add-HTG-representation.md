## Added a new HyperTreeGridRepresentation using the vtkOpenGLHyperTreeGridMapper

This representation handles 2D vtkHyperTreeGrid efficiently as it take benefits
from the vtkOpenGLHyperTreeGridMapper to only render the part of the Hyper Tree Grid
required by the current camera.
