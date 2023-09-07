## Cell Grid Support

### Developer-facing changes

ParaView now includes some basic support for the new `vtkCellGrid` subclass of `vtkDataObject`:
+ It can load JSON-formatted `.dg` files using `vtkCellGridReader`.
+ The `vtkPVDataInformation` and `vtkPVArrayInformation` classes now generate summary
  information for cell-grid objects and their cell-attributes.
+ Cell grids are renderable via a new representation that includes surface and
  outline representation-styles.
+ The `Convert to Cell Grid` filter will convert unstructured grids (optionally with
  special markup data) into cell grids.

### User-facing changes

If you have or create `.dg` files with discontinuous data, you can now load and render
them in ParaView.

### Known issues and missing features

+ Choosing components of a cell-attribute to color by does not work.
+ Rendering with an opacity < 1.0 is slow due to forced re-uploads of arrays to
  the GPU each frame.
+ Selection of cell-grid data (blocks, points, cells) is not implemented yet.
