ParaView 5.7.1 Release Notes
============================

Major changes made since ParaView 5.7.0 are listed in this document. The full list of issues addressed by this release is available
[here](https://gitlab.kitware.com/paraview/paraview/-/milestones/16).

# General Notes

This patch release is mostly dedicated to various file IO bugs that were found in the 5.7.0 release. It does however have one new feature, a plugin that converts data into a tree based AMR format.

### New Features

* The new 'HyperTreeGridADR' plugin adds a 'Resample To HyperTreeGrid' filter that converts any data set into a tree based AMR data type. It does so by doing intelligent resampling that optionally takes into account attribute expressions to control the refinement process. The algorithm follows the approach outlined in the Nouanesengsy et al's Analysis-Driven Refinement paper from LDAV 2014.
* ParaView can be now compiled with OpenImageDenoise releases newer than 0.8.
* The Web exporter for standalone data viewing now preserves dataset names.

### Bug fixes

* There are a number of miscellaneous refinements to 5.7.0's new cmake infrastructure.
* A crash discovered in 5.7.0's Cell Size filter is fixed.
* Two regressions in which the HyperTreeGrid .htg writer did not support multicomponent arrays and could not be written into .vtm collections are fixed.
* There are a number of miscellaneous bug fixes to HyperTreeGrid filters.
* Two bugs are fixed in the AMReX (formerly BoxLib) reader.
* The MPAS reader no longer issues warnings when reading datasets without time.
* Inconsistent time information and missing cell arrays in the Truchas reader are fixed.
* Incorrect 2D rectilinear grid orientation with both XDMF readers are corrected.
* The brand new ExportNow feature properly skips screenshots from deselected views in this release.
* Catalyst scripts with Cinema image exports no longer skip attribute arrays.
