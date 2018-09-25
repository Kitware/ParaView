PanoramicProjectionView plugin for ParaView
===========================================

by M.Migliore & J.Pouderoux, Kitware SAS 2018

This work was supported by Helmholtz-Zentrum Geesthacht.

Description
-----------

This plugin provides a new render view type called *Panoramic Projection View*.
This new render view projects the current scene in order to capture the scene up to 360 degrees.
Produced images can be used as input for specific devices like VR headsets, domes, panoramic screens
or to produce spherical movies.
Two projection types are available:

* Equirectangular projection (usual world map projection)
* Azimuthal equidistant projection (fisheye projection)

How to test
-----------

* Load the plugin "PanoramicProjectionView".
* Create/load a dataset.
* Create a new *Panoramic Projection View*
* Choose the projection type and the vertical angle in the view properties
