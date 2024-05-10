ParaView 5.12.1 Release Notes
=============================

Bug fixes made since ParaView 5.12.0 are listed in this document. The full list of issues addressed by this release is available
[here](https://gitlab.kitware.com/paraview/paraview/-/milestones/27).

## Rendering

* Gradient backgrounds in saved images larger than the render window are now correct. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22559)

* Support for OSPRay is checked only when OSPRay rendering is requested. Previously, it was checked unconditionally and that could lead to a valid but unwanted warning when ParaView runs on a system that does not support OSPRay. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22609)

* Cached geometry is discarded whenever an animation keyframe is edited. This can prevent spurious visualization of geometry when keyframes that modify filter properties are changed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6831)

* Coloring by _partial_ field data arrays now works correctly. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6690)

## Filters

* The **Reflect** filter on triangle strips now produces correct cells. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21321)

* Slicing image data with multiple components now correctly copies all components to the output. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22507)

* Fixed a crash when loading an unstructured grid containing a VTK_CONVEX_POINT_SET from a VTU file. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22546)

* Removed spurious warning message when the **Integrate Variables** filter was applied to an empty dataset. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22563)

* The **Legacy Ghost Cells Generator** filter, which was deprecated in ParaView 5.10 and removed in ParaView 5.11, has been added back. It is available after loading the LegacyGhostCellsGenerator plugin. This filter may work better on large scale runs on particular HPC systems where MPI resources are exhausted when using the regular **Ghost Cells Generator**. In addition, the **Legacy Ghost Cells Generator** has a property **Minimum Number of Ghosts** that matches the name of a property in the **Ghost Cells Generator**. This makes swapping between the two filters in a Python script simpler. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22502)

* The **Gradient** filter had a thread race condition fixed [(details)](https://gitlab.kitware.com/vtk/vtk/-/issues/19312)

## Readers

* VTK XML file format readers no longer fail when building ParaView against expat 2.6.0. [(details)]()

## Writers

* Saving a multiblock dataset file (VTM) in parallel now uses only rank 0 to create a subdirectory instead of all ranks trying and one rank winning. This resolves a race condition on some file systems where the directory was reported as being created but was not available for writing yet by one of the processes. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22579)

## General

* Fixed state saving option when a remote server crashes. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22529)

## Catalyst

* Error reporting when creating extracts directory fails now includes the full path of the directory whose creation failed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6713)

## Build

* osmesa is now always built with gcc in the superbuild to avoid core dumps, even when building ParaView with an Intel compiler. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21767)

* Older versions of Python are again supported. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22401)
