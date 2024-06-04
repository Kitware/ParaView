## Short descriptive title starting with two hashes and capitalizing only the first word and titles

This is a sample release note for the change in a topic.
Developers should add similar notes for each topic branch
making a noteworthy change.  Each document should be named
and titled to match the topic name to avoid merge conflicts.

Tips for writing a good release note:

* Prefer active voice:

  ** Bad: A function to X was added to ParaView.

  ** Good: ParaView now provides function X.

* Write to the user, not about the user.

  ** Bad: Users can now do X with Y.

  ** Good: You can now do X with filter Y.

* Avoid referring to the current commit or merge request

* Add images to Documentation/release/dev

* Use the ParaView name for dataset types instead of VTK class names:

  * Polygonal Mesh instead of `vtkPolyData`
  * Image instead of `vtkImageData`
  * Structured Grid instead of `vtkStructuredGrid`
  * Rectilinear Grid instead of `vtkRectilinearGrid`
  * Unstructured Grid instead of `vtkUnstructuredGrid`
  * Multi-block Dataset instead of `vtkMultiBlockDataSet`
  * Overlapping AMR Dataset instead of `vtkOverlappingAMR`
  * Hyper-tree Grid instead of `vtkHyperTreeGrid`
  * Table instead of `vtkTable`
  * Partitioned Dataset instead of `vtkPartitionedDataSet`
  * Partitioned Dataset Collection instead of `vtkPartitionedDataSetCollection`

* Avoid referring to "exposing" VTK filters or VTK filter names. Refer instead to the parts of ParaView that your change has affected:

  ** Bad: Exposed a new property from `vtkPVGeometryFilter` in ParaView.

  ** Good: The **Extract Surface** filter has a new property.

* Do not add line breaks to paragraphs of text.

* To add images, use the following pattern:

> ![Alternative text describing the image](image.png)
>
> Caption for the image

* Python the language should always be capitalized

* Formatting conventions for different types of things in ParaView

  * Menus - put menu names between backticks with arrows pointing to submenu items, e.g., `File -> Save Data`.
  * Proxy properties - **bold**, and the name should match the name presented in the UI, e.g., **Split Fraction**.
  * Representations - **bold**, and the name should match the named presented in the UI, e.g., **Surface With Edges**.
  * Source/Filter/Extractor names - **bold**, and thename should match the name presented in the UI, e.g., **Surface Normals**.
  * UI elements - _italicized_, e.g., _Python Shell_.
  * Plugins - no special formatting.
