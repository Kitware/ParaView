## Cleanup vtkCaveSynchronizedRenderers

The vtkCaveSynchronizedRenderers class has been cleaned up.

In that context ComputeCamera has been deprecated in favor of InitializeCamera and the following protected members are now private.
 - double EyeSeparation;
 - int NumberOfDisplays;
 - double** Displays;
 - double DisplayOrigin[3];
 - double DisplayX[3];
 - double DisplayY[3];
 - int once;
