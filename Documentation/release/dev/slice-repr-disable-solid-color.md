## Disable "Solid Color" for representations that handle it poorly.

Previously, when the "Solid Color" option was chosen from the `pqDisplayColorWidget`,
the "Slice" representation would print an error and use the active scalars anyway. The
"Volume" representation will show an empty visualization.

The `NoSolidColor` hint is provided in the representation XML so that the
`pqDisplayColorWidget` will disable the "Solid Color" option when present.
