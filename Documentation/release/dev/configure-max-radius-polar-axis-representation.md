## Configurable max radius polar axis representation

You can now configure a custom max radius for polar axis representation.
Note that by default the custom max radius is off and therefore  automatically computed relatively to pole position.

The protected member variable `vtkPolarAxesRepresentation::EnableCustomRadius` and its related methods
`SetEnableCustomRadius` and `GetEnableCustomRadius` have been deprecated.
They are replaced by `SetEnableCustomMinRadius` and `GetEnableCustomMinRadius` methods to maintain consistency
in naming.
