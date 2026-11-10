## Side data on unstructured grids

ParaView now supports side data on an Unstructured Grid: data defined on the sides shared between cells, that is the faces of 3D cells and the edges of 2D cells. A side shared by two cells holds one value, which makes it the natural place for quantities such as the normal velocity through a face of a staggered-grid ocean or atmosphere model.

When an Unstructured Grid carries side data:

* The _Information_ panel shows the number of sides and lists the side arrays.
* The _Spreadsheet View_ offers **Side Data** as an attribute, with one row per side.
* The **XML Unstructured Grid Reader** lists the side arrays of a `.vtu` file under **Side Arrays**, so you can choose which ones to read. The **XML Unstructured Grid Writer** writes them, and the **Pass Arrays** filter and the writers' array selection have a **Side Data Arrays** list.
* The **Python Calculator** and **Python Annotation** filters accept **Side Data** as their array association, and Python scripts can reach the side arrays of a source through `source.SideData`.

Side arrays cannot be used for coloring yet, so they are not listed in the coloring array selector.

### Developer notes

`vtkSMInputArrayDomain` has a new `SIDE` attribute type and `attribute_type="side"`. Side arrays are only matched by `side`, not by `any` or `any-except-field`. `vtkSMFieldDataDomain` lists side data only when the XML sets `enable_side_data="1"`, since most filters do not process side data. `vtkPVDataInformation` has `GetSideDataInformation()` and `GetNumberOfSides()`.
