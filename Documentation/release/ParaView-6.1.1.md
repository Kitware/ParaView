ParaView 6.1.1 Release Notes
=============================

Bug fixes made since ParaView 6.1.0 are listed in this document. The full list of issues addressed by this patch release is available [here](https://gitlab.kitware.com/paraview/paraview/-/milestones/36).

## Screenshot to clipboard fixed

ParaView now properly supports screenshoting to clipboard, which was not working in v6.0.X. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23271)

## Incorrect Above Range Color and Below Range Color mapping in color legend fixed

Data values very near the color legend data range could be incorrectly classified as above or below the data range due to floating point rounding errors. Fixed this by doing comparisons with the raw data. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13144)

## Fixed crash when Convert To Point Cloud has null points in input

When the **Convert To Point Cloud** filter had an input `vtkPointSet` with a null `vtkPoints`, it would crash with a segmentation fault. That has now been fixed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/22724)

## Fixed Python Shell unresponsiveness after specific commands and auto completer usage

Added code to handle Python exceptions during code completion that could lead to the _Python Shell_ being unresponsive. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/22969)

## Fixed compilation with an external VTK

Updated the VisItBridge with a CMake fix to make building with external VTK work. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23235)

## MomentInvariants compilation fix

Fixed compilation error in the MomentInvariants module. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13173)

## HyperTreeGrid Evaluate Coarse

This filter's performance has been improved and a race condition was fixed. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13108)

## IOSS-related bug and performance fixes

A bug that prevented the IOSS reader from reading Exodus files exported by the code Alegra has been fixed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23256)

Performance of the IOSS Exodus reader has been greatly improved by changing the order that blocks are read from files. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/22265)

Fixed a bug when the **Merge Exodus Entities Blocks**  property was enabled. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13101)

IOSS reader now provides a warning when fields are not the same. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13206)

## Legacy Exodus reader crash fixed

The crash with the Legacy Exodus reader could crash with certain datasets is now fixed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23254)

## Fixed EnSight Gold file reader not recognizing certain case files

Some valid EnSight Gold case files that were not recognized by ParaView 6.1.0 are now recognized and load correctly. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23253)

## Fixed CDI Reader 3D mask allocation

The CDI Reader now correctly handles memory allocation for multilayer 3D mask data. Previously, it allocated based on maximum vertical levels rather than the actual number of levels in the dataset, causing out-of-bounds access. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23246)
