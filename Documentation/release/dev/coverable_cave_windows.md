## Coverable CAVE (pvserver) windows

Adds a new attribute, "Coverable", available to set on `Machine` elements in a `.pvx`
file.  When the attribute is set to 1, the associated `pvserver` window will be
manageable by the windowing system.  This allows other windows to cover it, and
provides a taskbar item which can be used to bring the `pvserver` window back to
the front.

This functionality is only available for the X windows implementation, a warning
will be printed on non-X windows systems.
