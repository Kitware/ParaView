## Merge vtkSession and vtkPVSession classes

`vtkSession` and `vtkPVSession` classes are now merged to resolve naming conflicts in install tree
caused by the introduction of a new `vtkSession` header in VTK.

In that context, a deprecated `vtkSession` class is provided for retro-compatibility and
the `vtkSessionIterator` class is deprecated in favor of `vtkPVSessionIterator` to maintain consistency.
