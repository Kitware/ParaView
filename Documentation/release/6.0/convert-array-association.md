## Convert Array Association

A filter may require an input array associated to a specific attribute
type (mainly Points or Cells). ParaView provides a global settings to automatically
move arrays to the required association. This applies to every filter.

It is now possible to specify this behavior per filter in the XML
configuration with the attribute `auto_convert_association="1"` in
the `InputArrayDomain`.

The conversion is done if either the global settings is true or the
filter property is set to "1".

The **Contour** filter uses this feature.

### Notice

With automatic array conversion, more arrays are available.
This may change the default array picked by the downstream filter.
