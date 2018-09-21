# Add user setting for default Render View interaction mode

A new user setting is added to render views for the 2D/3D interaction
mode to use when loading a new dataset. The default behavior, called
"Automatic", is the current ParaView logic, in which the interaction mode
is set to 2D or 3D based on the geometric bounds of the dataset.
New options are now available to override this, and alternatively specify
"Always 2D" or "Always 3D".
