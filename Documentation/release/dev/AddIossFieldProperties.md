## Add IOSS field properties.

You can now toggle the `IGNORE_REALN_FIELDS` property from the
IOSSReader property panel. You can also specify a string for the
`FIELD_SUFFIX_SEPARATOR` property.

When `IGNORE_REALN_FIELDS` is
1. off - numeric suffices are treated as the components of a vector.
  names field_1, field_2, field_3 will result in a 3-component vector field.

2. on  - numeric suffices are not treated as components of a vector.
  names field_1, field_2, field_3 will result in three scalar fields.

By default, the IOSS reader ignores real[n] fields and the field suffix
separator is empty.
