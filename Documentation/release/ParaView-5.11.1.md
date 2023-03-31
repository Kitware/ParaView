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

vtkSMFieldDataDomain: Fix default_values selection [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6248)

Fix a potential segfault by adding sanity check in pqAnimationViewWidget [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6204).

vtkSMSelectionHelper: Remove usage of regex [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6199).

calculator: Check if association is not None [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6201).

Add catalyst cxx11 check [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6200).

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

Multiblock inspector color and opacity default to 1 [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/6054).

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

Fix vtkLagrangianParticleTracker members initialization [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9856)

OSPRAY private link instead of public [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9853)

Only pass ContextScene mouse event when item is interactive [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9896)

Makes RenderingContextOpenGL2 module buildable with Emscripten [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9904)

vtkPointSet returned in vtkDataObjectTypes::NewDataObject [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9902)

vtkExtractBlockUsingDataAssembly sanity check [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9900)

vtkPolygon: fix normal computation instability [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9870)

X11 `ProcessEvents` will dispatch all pending messages [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9913)

Avoid setting null shader in PolyDataMapper [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9923)

Fix missing symbol on Emscripten [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9915)

Fix SSAO render pass [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9907)

Fix magnifier viewport with multiple renderers [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9898)

Minor progress reporting fixes for vtkThreshold and vtkContour filters [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9836)

vtkOBJReader: avoid a possible null buffer access [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9827)

vtkPLYWriter: Fix can't change the texture coordinates name after writing data once [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9978)

Resolve stuttering dolly with SDL2 interactor in browsers [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9993)

Allow to change cursor in QVTKOpenGLStereoWidget [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9985)

Fix PDAL link on windows [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9991)

vtkEnSightWriter: write .case file in RequestData [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9712)

ExprTkFunctionParser: Fix missing sstream include [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10004)

Fix delayed title update [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10029)

Fixed integer size to match what ex_get_block() requires [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10053)

Fixed / suppressed cppcheck 2.10 warnings [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10049)

Fix io xml htg writer and reader v2 [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10067)

vtkConstrainedPointHandleRepresentation - incorrectly checking this->CursorShape [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10075)

Trivial casts [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9966)

Make Cocoa context-view connection before MakeCurrent [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9845)

vtkStructuredDataPlaneCutter now pass field data [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10024)

Fix an incorrect usage of vtkDataAssembly::MakeValidNodeName in vtkDataAssemblyUtilities [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10023)

vtkCocoaRenderWindow: check if Initialized before calling Initialize()  [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10060)
