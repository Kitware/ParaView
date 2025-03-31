ParaView 5.13.3 Release Notes
=============================

Bug fixes made since ParaView 5.13.2 are listed below:

## Interface improvements

* _MultiBlock Inspector_ enables the _Extract Blocks_ button only when needed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7066)
* The _Lock View Size_ option in the _Tools_ menu is reset to off when ParaView's session is reset. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7130)
* _Find Data_: fixed an issue with regular expression of values containing a dot. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7189)
* Fixed an incorrect tooltip in the _Adjust Camera_ dialog. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7223)

## Rendering

* Fixed initial display of _Polar Axes_. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7071)
* Fixed _Polar Axes_ bounds in client/server mode. [(details)](https://gitlab.kitware.com/aron.helser/vtk/-/commit/acd3a1d8ded5e6a27e540bf5d47f667a4be401a0)
* Fixed issues drawing thick lines on Macs with Apple Silicon chips. (details [here](https://gitlab.kitware.com/paraview/paraview/-/issues/21832) and [here](https://gitlab.kitware.com/paraview/paraview/-/issues/22594))
* Unexpected rescaling of the bottom axis in the Histogram view was fixed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22733)
* Backface coloring for 3D Glyph representation has been fixed. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11789)
* The minimum and maximum values displayed when color legend's **Draw Data Range Label** option is on are now displayed correctly. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22661)

## Readers

* Fixed VTKHDF reader overflow issue when reading AMR datasets. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11593)
* AMReX Particle reader now works even when there is no header file available. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22835)
* Fixed AMReX Grid reader to enable reading files larger than 2GB on Windows. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11820)
* Fix AMReX Particles reader when using MPI when the number of grids in the loaded dataset is not
exactly divisible by the number of MPI processes. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11815)
* Fixed EnSight Gold binary reader to prevent integer math overflow when reading a large number of cells. [(details)](https://gitlab.kitware.com/griffin28/vtk/-/commit/814eba3a4d7ae35a75a552c488f2b13b06973451)
* XDMF2 reader now reports an error message when a dataset has a rank that does not match an array's selection rank. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/19614)
* The CDI reader is now more stable on Windows. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7202)

## Filters

* **Extract Selection Over Time** now works with HyperTree Grid datasets. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7196)

## Catalyst

* AnimationScene is no longer referenced in exported Catalyst scripts when there are no image extractors in the pipeline [(details)](https://gitlab.kitware.com/paraview/paraview/-/commit/bf50e57c1cc9ebef730e89bfa04e4d38c38f7eea)
* Fixed some flaky behavior when processing fields in Catalyst. [(details)](https://gitlab.kitware.com/paraview/paraview/-/commit/f0cff5281a4eae3cc4604026fa15d321cb89f8f7)
* Avoid crash when Catalyst 2 script path points to file that does not exist. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7128)
* ParaView Catalyst now catches `pipeline_execute_failed` errors that occur during Catalyst pipeline execution. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22756)

## Build

* Compilation failures on Intel Classic compilers was fixed. (details [here](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11928) and [here](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11277))
