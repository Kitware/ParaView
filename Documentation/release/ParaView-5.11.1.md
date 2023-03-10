ParaView 5.11.1 Release Notes
============================

Bug fixes made since ParaView 5.11.1 are listed in this document. The full list of issues addressed by this release is available
[here](https://gitlab.kitware.com/paraview/paraview/-/milestones/26).

# Notes

## Support for 12-node wedges in IOSS

The IOSS reader now supports mixed-order, 12-node wedge elements.
These elements have quadratic triangle faces but quadrilaterals
with 2 linear sides and are sometimes used to represent material
failure along a shared boundary between two tetrahedra (i.e.,
the wedge is assigned a zero or negligible stiffness, allowing
the neighboring tetrahedra to move relative to one another without
inducing strain.

Bump VTK to support vtkQuadraticLinearWedge in IOSS. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6215).

## Point Interpolator Null Points Strategy Typo Fix

The PointInterpolatorBase source proxy had a typo that prevented users from
selecting "Closest Point" as the Null Points strategy for the Point Line Interpolator,
Point Plane Interpolator, Point Volume Interpolator, and Point Dataset Interpolator
Filters. Note that this only affects those interpolation kernels that can produce
such null points, such as the Linear Kernel while using a radius based interpolation.

Bug: PointInterpolatorBase NullPointsStrategy Typo [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6177)

## pqLoadDataReaction bug with default file readers

pqLoadDataReaction was fixed to trigger the loadData() signal when a default reader was used as specified in Paraview-UserSettings.json.

pqLoadDataReaction: fix default reader logic [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6171)

# Bug fixes

Updating VTK to get critical VTK fixes [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6211).

Fix a potential segfault by adding sanity check in pqAnimationViewWidget [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6204).

Update VTK paraview/release branch [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6206).

vtkSMSelectionHelper: Remove usage of regex [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6199).

calculator: Check if association is not None [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6201).

Add catalyst cxx11 check [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6200).

Updating VTK paraview/release branch [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6192).

Fix BagPlot test [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6166)

Ignore all version of TextSourceBorder test in CI [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6152)

Exclude ComparativeViewOverlay test in CI [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6145)

Fix a small issue in array information [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6181).

Run Expression test in serial because of context menu [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6180).

proxies_citygml.xml: Fix unclosed bracket [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6172).

vtkPVGeometryFilter: Set Fast Mode off by default [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6154).

Add missing source_dir to extract_version_components cmake macro [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6151).

Remove incorrect vtklogger verbosity usage [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6122)

Fixup ServerConnectPluginLoad test [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6130)

Excluding some tests from CI [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6016)

Check timeKeeper pointer nullity before use. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6105).

Fix signal to signal connection for shortcuts [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6107).

Fix gcc 4.8 5 compilation- Part 1  [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6049).

Insade 2D - 3D transition [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6084).

Background palettes: adding tests [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6085).

gitlab-ci: update tags to include architectures  [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6079).

XRInterface: Fix properties initial values between successive XR views [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6067).

Fix LiveSource interval initialization [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6059).

Update VTK from paraview/release [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6056).

Multiblock inspector color and opacity default to 1 [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6054).

Update VTK with new fixes [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6046).

Updating VTK for vtkImageDifference fixes [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6041).

pqFileDialog: parent Favorites context menu [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6038).

ui: update help menu [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6027).

NodeEditor: add cmake option to disable Graphviz [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6021).

# VTK Bug Fixes

Support files with mixed-order, 12-node wedges. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10020)

vtkWin32Header: mask the `STRICT` preprocessor token [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9615)

Fix vtkPolyData::RemoveGhostCells field data handling [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10009)

ioss: update for -Wundef errors [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10000)

TreeInformation: sort via `const&` parameters [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10001)

Add #include <cstdint> to compile with gcc13 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9996)

Increase Celltree Locator Max Depth [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9994)

vtkExprTkFunctionParser: Remove regex usage [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9988)

Fix MetaIO ElementSpacing precision mismatch [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9980)

Make FindExprTk more robust [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9981)

QQuickVTKInteractorAdapter: support Qt 5.9 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9972)

vtkUnstructuredGrid: remove deprecation of GetCellTypes [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9965)

insert was returning an iterator in gcc 4.8.5 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9970)

Updating VTKm [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9957)

Fix a small issue in the LPT regarding max nSteps [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9941)

vtkIOSSReader: Fix same structure assumption for EXODUS format [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9921)

vtkGeometryFilter: Remove original ids arrays [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9920)

vtkGeometryFilter: Fix ijk assignment for non fast-mode [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9884)

fides: check for empty input strings [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9881)

vtkConduitSource.cxx: fix typo in conditional [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9863)

Remove use of importlib from vtk.py [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9865)

vtkSelection: Check if numVals != -1 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9851)

Add missing include to algorithm for std::min/max [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9854)

Fides bug fixes [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9805)

vtkDataSetSurfaceFilter: Fix segmentation fault [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9832)

vtkXMLTreeReader: protect against possible nullptr dereference [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9822)

Fix accidental breakage of interactive X resizing [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9813)

Add vtkmodules/web/errors.py [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9814)

Release v9.2.3 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9808)

Wep/Python: move Python dependencies to extras_require [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9727)

Xdmf2: Update HDF5 driver for 1.13.2 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9690)

gitlab-ci: remove `linux-3.17` tag [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9789)

vtkSMPToolsImpl: Remove second constructor [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9797)

Move check for visibility to leaf translucency check [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9775)

numpy: avoid `numpy.bool` in MPI-using code [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9795)

Fix GCC 4.8 5 compilation [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9728)

Update Web render window serializers and remove print statements [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9766)

vtkModuleTesting: fail Python tests on deprecation warnings [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9788)

Improve PBR textures [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9786)

Update IO/IOSS/vtkIOSSReader.cxx [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9724)

vtkPythonInterpreter: fix mismatched malloc/free usage [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9771)

VTK v9.2.3 release note updates [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9761)

Update IOSS 2022-12-14 (540f30cc) [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9753)

Prevent the use of ISB on ARM platforms that dont support it [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9751)

Update libproj 2022-12-13 (c9761326) [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9754)

matplotlib: support 3.6 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9750)

deprecated apis: avoid deprecated FFMpeg and Python APIs [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9743)

vtkVRRenderer: Fix hard coded translation when resetting camera [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9741)

Fix LagrangianParticleTracker cache [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9740)

Fix cell data allocate clip-slice [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9731)

Fix ambiguous overload error with abs [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9735)

Update mpi4py 2019-11-27 (55be1ff2) [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9737)

Add missing vtkMatplotlibMathTextUtilities [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9722)

Fix vtkImageDifference [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9717)

## Release backports

[vtk!9856](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9856)
[vtk!9853](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9853)
[vtk!9896](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9896)
[vtk!9904](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9904)
[vtk!9902](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9902)
[vtk!9900](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9900)
[vtk!9870](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9870)
[vtk!9913](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9913)
[vtk!9923](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9923)
[vtk!9915](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9915)
[vtk!9907](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9907)
[vtk!9898](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9898)
[vtk!9836](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9836)
[vtk!9827](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9827)
