## `Stream Tracer With Custom Source` Supports Cell Data

Previously, the `Stream Tracer With Custom Source` could not be applied to a
data set with velocity vectors in a field associated with cells. However, the
underlying stream tracer field does support such data.

The `Stream Tracer With Custom Source` filter specification has been updated to
allow velocity vectors in cell data. This prevents unnecessary conversions of
cell data to point data.
