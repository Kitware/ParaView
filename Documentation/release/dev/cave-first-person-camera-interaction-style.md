## New First Person Camera interactor style in CAVE plugin

The First Person Camera is a new interaction style that binds the mouse and the keyboard to enable movement like a first person flight by changing
the view direction of the camera with the mouse and by using four keys of the keyboard to move it in the space. To function correctly, the controlled
proxy must be a Render View and since it definetly controls the camera, there is no controlled property to set. The input device must return the
absolute position of the mouse on the screen as valuators, one for X position and one for Y position (a `vrpn_Mouse` for example with VRPN).
In the CAVE, this interaction style won't have any effect unless the off-axis projection is turned off in the CAVE configuration.
