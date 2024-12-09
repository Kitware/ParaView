## Exposes cell quality filter

ParaView now exposes a new filter `Cell Quality` which allows you to compute a given cell
quality metric to all cell containing in a DataSet.

As some metrics or cell types can be not supported, you can set a default value for such cases with:
- `UndefinedQuality` : for undefined measure, default is -1.
- `UnsupportedGeometry` : for unsupported cell type, default is -2.
