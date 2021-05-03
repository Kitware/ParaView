ParaView 5.9.1 Release Notes
============================

Bug fixes made since ParaView 5.9.0 are listed in this document. The full list of issues addressed by this release is available
[here](https://gitlab.kitware.com/paraview/paraview/-/milestones/21).

## Catalyst

* Fixed Catalyst build error on Windows with CMake 3.15 and before [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4894).

* Fixed Catalyst V2 API to avoid adding the same timestep twice and getting warnings about it [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4792).

* Fixed issue with Catalyst V2 scripts missing time at first call [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4700).

* Ensure all fields are requested in time step 0. When using V2 Catalyst Python scripts, since the script is not imported when the first `RequestDataDescription` is called, we need to ensure all fields/meshes are requested. The comment already mentioned that, we were just toggling all the necessary flags for the fields to be requested correctly [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4688).

##  Reader changes

* Added HDF5-based XRage reader. This reader loads HDF5 data produced by xRage [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4851).

* Several fixes for the PIO reader. Default selected variables have been fixed [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7849). Fixed a bug where sometimes a variable was determined to be a string when it was actually numerical data. Enabled comments that start with '!' and '#'. Improved some error messages [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7655).

* EnSight Gold reader now ignores 'maximum time steps:' lines in .case files [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7798).

* Fixed bug in NetCDF reader where only the first array would be displayed in the array selection [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7550).

* Motion FX reader now has a universal transform [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7666).

## Ray tracing fixes

* Ray tracing is initialized upon request, not at program startup, to avoid the cost of initialization and potentially platform-specific problems encountered when ray tracing is not requested [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4743).

* On macos, OSPRay now detects whether it is running on Rosetta and reports that it is not available instead of crashing [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4710).

* Silenced a JSON parser warning issued when an OSPRay-enabled client connected to a server built without OSPRay support [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4704).

* Fixed a bug where once made visible, axes labels would always be visible under ray tracing [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4595).

* Reduce severity of VisRTX warning messages to avoid extraneous output [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4733).


## Bug fixes

* The **Annotate Global Data** filter now validates the format string and warns when the format is not valid for the selected array type [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/20612).

* The **PassArrays** filter now no longer passes "vtkGhostType" arrays if they are not in the array selection. Previously, the **PassArray** filter would pass "vtkGhostType" arrays even if they were excluded from the "*DataArrays" properties [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/20379).

* A warning that could appear during interactive cell selection with labels turned on has been fixed [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/20548).

* Fixed a segmentation fault that could occur when disconnecting from a remote server [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4773).

* cinemasci: use relative paths when loading modules [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4734).

* The **Surface LIC** representation adds backface representation support [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7535).

* Fixed **Surface LIC** representation when the data has a Normals array [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7793).

* Fixed memory leak in value pass [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7836).

* Subdivision for non-linear curves has been implemented [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7837).

* Array buffer and element array buffer bindings are now cached in OpenGLState.

* Disabled progress events in the "Projected tetra" volume rendering algorithm. This speeds up rendering and prevents problems caused by code executed when progress events are invoked [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7796).

* Re-enable point merging when requested for unstructured grids [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7659).

* Updated `vtkSphereTree` to use thread-safe method to get cell IDs [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7636).

* Fixed some compiler errors on IBM XLC compilers [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7591).

* NVIDIA IndeX plugin bugfixes [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4910)

  * Fixed loading of the NVIDIA IndeX plugin in `pvbatch` when `PV_PLUGIN_PATH` is set [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/20487).

  * Fixed "z-buffer precision" error message in the NVIDIA IndeX plugin when the Axes Grid was enabled.

  * Fixed NVIDIA IndeX rendering  when the color map was rescaled to a custom range.

  * Runtime compilation of NVIDIA IndeX Visual Elements is now faster.

  * Prevent runtime issues in the NVIDIA IndeX library when a stub version of `libcuda.so.1` is in the library search path.
