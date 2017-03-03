StreamLinesRepresentation plugin for ParaView
=============================================

by B.Jacquet & J.Pouderoux, Kitware SAS 2017

This work was supported by Total SA.

Description
-----------

This plugin provides a new representation called "Stream Lines" for DataSet
in ParaView. The representation displays an animated view of streamlines in
a vector field of the dataset. Seeds are initialized randomly in the bounds
of the domain and created each time a particle dies (time to live - ie, max
number of iterations - is reached, out-of-domain or zero velocity).
The UI panel (ie. proxy) allows to specify:
* Vectors: the vector field (mandatory).
* Alpha: the rate of blending (depends on MaxTimeToLive, 0: no trace, 1: trace
  will face as long as TimeToLive).
* Step Length: Normalized integration step - allow to adjust particle speed.
* Number Of Particles: Number of simulated particles in the flow.
* Max Time To Live: Maximum number of iteration a particle is followed before
  it dies.
The solid color and line width can be changed using default ParaView UI widgets.

Note that pqStreamLinesAnimationManager class observes all pqRenderView. When a
rendering on such a view is finished, it checks all existing representations
and search for an enabled StreamLines one. If found, a new still render pass is
requested. This mechanism allow to refresh the view and animate the particles.

How to test
-----------

Load the plugin "StreamLinesRepresentation".
Create a "Wavelet" source. Apply a "Gradient" filter on it.
Switch representation from "Outline" to "Stream Lines" (apply a Tethrahedralize
filter to try it with Unstructured data).
Play with the "Stream Lines" properties in the Representation panel.
Another nice demo is to load the disk_out_ref.ex2 dataset with the V vector
point data array.

Known bugs/limitations
----------------------

* Does not work in remote rendering with parallel server

Potential extension features
----------------------------

* For non-AABB (based on vol(domain)/vol(AABB) < 0.2): better point sampling via
  cell-sampling using cumulative sum of cell-volume
* Take benefit of the SMP features (ie. update particles in parallel)
* Kill out-of-frustum particles
