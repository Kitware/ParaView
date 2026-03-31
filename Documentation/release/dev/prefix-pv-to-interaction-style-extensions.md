## Interaction style extension classes are now prefixed with `PV`

VTK class names in the InteractionStyle extensions have been prefixed with `PV` to avoid naming conflicts.

The following classes have been renamed:
* `vtkCameraManipulator` -> `vtkPVCameraManipulator`
* `vtkTrackballPan` -> `vtkPVTrackballPanAxisConstrained` (because `vtkPVTrackballPan` was already used for a different class)

This is an unavoidable breaking change for developers who have been using these classes in their code, but it should not affect direct users of ParaView. It was necessry since VTK is now providing its own `vtkCameraManipulator` class, which is going to be a replacement for `vtkPVCameraManipulator` in the future. The new VTK class is not a subclass of `vtkPVCameraManipulator`, so the two classes will need to co-exist for some time. The `PV` prefix makes it clear which class is which and avoids naming conflicts.
