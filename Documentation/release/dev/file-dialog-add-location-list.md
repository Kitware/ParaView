## Add Locations list to pqFileDialog

Include a "Locations" list to the left side of the pqFileDialog. It is updated
every time the dialog is created with "special" directories, which includes
drives. This resolves an issue on Windows, because the Favorites list used to
show these special directories, but it is saved in the settings and not updated
once created, so new drives like USB keys or newly mapped network drives don't
appear.

The Favorites list is now empty by default, since special directories can easily
be added from the Locations list.

A "Media" special directory is added on Linux for common directories where USB
or other auto-mounted drives are typically located. The "Downloads" special
directory is added on Windows, and the method for retrieving special
directories is updated. The "Examples" place-holder directory is also added to
special directories by default, but can be controlled with a behavior.

A new setting in the Misc section allows the user to specify a list of labels
and environment variables. Each of the env-var values is looked up, and if it
exists, the path it contains is added to the Locations list. If the path
doesn't exist, the warning icon is used. If the env-var is not set or is
empty, the entry is not added to the Locations list.
