ParaView 5.10.1 Release Notes
=============================

ParaView 5.10.1 is a patch release with a variety of bug fixes. The full list of issues addressed by this release is available [here](https://gitlab.kitware.com/paraview/paraview/-/issues?scope=all&state=closed&milestone_title=5.10.1%20(Winter%202022)).

## User interface

* State files appear as datasets. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/18000)

* File dialog issues in client/server mode [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21214)

* Stride in Animation View is too restricted[(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21183)

* Context menu in file dialog doesn't work on macOS with Qt 5.15.1 [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/20981)

* Python tracing fails when a custom source exists [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21290)

* _Python Shell_ broken by any custom filter [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21090)

* Stereo broken in CAVE environments [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21287)

* Parallel Coordinates segfault [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21258)

* Can't set frame rate when saving MP4 animation [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21212)

* `command_button` widget is not visible anymore [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21173)

* Client information does not show VTK version. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21074)

* Only fetch favorites from server on creation [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/5520)

## Filters

* Ghost cell generator crash [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21228)

* Redistribute dataset on Image Data then generate Ghost cells seems to give the wrong result [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21161)

* TemporalParticlesToPathlines fails with AMReX particle data: "The input dataset did not have a valid DATA_TIME_STEPS information key" [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21162)

## Readers

* Sideset error with IOSS reader [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21185)

* IOSS reader is slow with small, parallel dataset [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21172)

* IOSS reader and set variables don't work [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21231)

* Restart file fails with IOSS reader [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21191)

* PIO reader seg faults on warnings for client-server [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21235)

* PIO reader fails on extra file in directory [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21263)

* PIO reader access to restart block and even/odd checkpoints [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21206)

* openPMD: Fix Particle Time Series [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/5539)

## Catalyst

* Multiple grids with multiple pipelines produces failure in coprocessing.py [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21225)

* SaveExtract volumetric cinema database crash [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21105)

## Build

* Guard the list of required vtk components for paraview package. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21171)

* `pqPythonUtils.h` not installed [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/21170)
