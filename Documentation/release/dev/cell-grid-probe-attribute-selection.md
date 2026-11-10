## Select the cell-grid attributes to probe

The `CellGrid Probe` filter now has a `CellGridAttributes` property to select
any number of the cell-grid attributes to interpolate to the probe points,
including none to only find the points that lie inside the cells.
It replaces the `CellGridAttribute` property, which could select at most one
attribute.

Saved states using `CellGridAttribute` are converted when loaded, and Python
scripts using it keep working with the compatibility version set to 6.2 or
older.
