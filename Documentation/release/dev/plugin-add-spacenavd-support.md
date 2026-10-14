## Added spacenavd support for 3D connexion spacemouse on Linux

The SpaceMouseInteractor Plugin now works on Linux with the Open Source drivers
and libraries [FreeSpacenav](https://github.com/FreeSpacenav). On Linux a user
can now just use the space mouse.

To build the Plugin for Linux [libspnav](https://github.com/FreeSpacenav/libspnav)
needs to be installed on the system (no X11 support necessary). The Plugin was
tested with libspnav's [Release 1.2](https://github.com/FreeSpacenav/libspnav/releases/tag/v1.2)
(from March 30th 2025).

For the plugin to work [spacenavd](https://github.com/FreeSpacenav/spacenavd)
must be running on the system.
