## Improvements to ordered compositing

Ordered compositing is used whenever ParaView is rendering
translucent geometry or volumetric data in parallel. For ordered
compositing, the data in the scene needs to be redistributed across
all rendering ranks to ensure the ranks has non-overlapping datasets with
other ranks. The data redistribution code has been revamped. It now
internally uses the `vtkRedistributeDataSet` filter. This greatly improves
performance when volume rendering unstructured grids in parallel. The code is also
much simpler. However, this introduces non-backwards compatible API changes that may
impact certain custom representations. Please refer to the `MajorAPIChanged` document
for details.
