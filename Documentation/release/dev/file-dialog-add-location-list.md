## Add Locations list to pqFileDialog

Include a "Locations" list to the left side of the pqFileDialog. It is updated every time
the dialog is created with "special" directories, which includes drives. This resolves an
issue on Windows, because while the "Favorites" list includes drives by default, the
favorites list is saved in the settings and not updated once created, so new drives like
USB keys or newly mapped network drives don't appear. Users can also remove drives from
the favorites list, and now they will still appear in the Locations list.

A "Media" special directory is added on Linux for common directories where USB or other
auto-mounted drives are typically located. The "Examples" place-holder directory is
also added to special directories by default, but can be controlled with a behavior.
