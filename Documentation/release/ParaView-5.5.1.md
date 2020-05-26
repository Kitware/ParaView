ParaView 5.5.1 Release Notes
============================

ParaView 5.5.1 is a patch release. It addresses the following issues:


* The [Exodus library](https://github.com/gsjaardema/seacas/tree/master/packages/seacas/libraries/exodus)
 in VTK has been upgraded to version 7.13. This version of the library contains a fix for a major issue encountered when loading side set geometry.

* A bug in the **Ghost Cell Generator** that could lead to a segmentation fault
has been fixed.

* Minor fixes to the camera model for the Cinema import and exporter have been added.

* Filters that are not equipped to handle `vtkHyperTreeGrid` data sets return
an error rather than crash.

* The `vtkXMLStructuredDataWriter` has been fixed to produce valid files when
appending time-varying data.

* The SDK packaging for Linux has been fixed.

The list of issues addressed by this release are available
[here](https://gitlab.kitware.com/paraview/paraview/-/milestones/11#tab-issues),
and merge requests included in this release are [here](https://gitlab.kitware.com/paraview/paraview/-/milestones/11#tab-merge-requests).
