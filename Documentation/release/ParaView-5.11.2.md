ParaView 5.11.1 Release Notes
============================

Bug fixes made since ParaView 5.11.1 are listed in this document.

## User interface

* pqFileDialog: Remove existence check when selecting multiple files  [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22230)

* Avoid crash in Find Data panel if nothing is selected [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6405)

## Remoting

* ArrayListDomain: Fix a potential segfault with empty array name [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6285)

* RangeDomain: fix interval condition [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6286)

* Add sanity checks around several GridAxesRepresentation usage
  * [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6273)
  * [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6326)

## Filters

* Support jpeg extension for textures.  [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22259)

* vtkHyperTreeGridAxisClip: Fix InsideOut [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6296)

## Readers

* openPMD: The openPMD python module was updated to fix bugs. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6275)

* Incorrect fetching of rectilinear grid object [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/14682)

## Catalyst

* Fixing Catalyst2 example to have proper Conduit spacing names [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6407)

## Python

* pythonalgorithm: Fix for smproperty.proxy [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6393)

* Fix --displays gets passed to pvbatch [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6307)

## Rendering

* Volume rendering of tetrahedral meshes is now fixed on some Macs [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10245)
