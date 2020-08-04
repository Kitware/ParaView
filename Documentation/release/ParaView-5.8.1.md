ParaView 5.8.1 Release Notes
============================

* ParaView 5.8.1 is a bug fix release that addresses 60 issues identified since the ParaView 5.8.0 release. The full list of resolved bugs with links to the bug reports is provided below.

## Readers and filters

* [AMR Dual Clip and Contour Accept invalid input datasets	](https://gitlab.kitware.com/paraview/paraview/-/issues/12478)
* [Parallel Resample to Image pipeline doesn't propagate all data	](https://gitlab.kitware.com/paraview/paraview/-/issues/18590)
* [Clip filter: Crinkle clip performs a slice rather than a clip	](https://gitlab.kitware.com/paraview/paraview/-/issues/19641)
* [AMReX Boxlib Grid Reader fails to the read coordinate range	](https://gitlab.kitware.com/paraview/paraview/-/issues/19692)
* [Paraview 5.8.0-RC3 VTK-xml reader: cell offsets cant be unsigned ints	](https://gitlab.kitware.com/paraview/paraview/-/issues/19698)
* [amrex: failed to read amrex 2D particles	](https://gitlab.kitware.com/paraview/paraview/-/issues/19776)
* [VTP Loading Apparent Regression 5.6.0->5.8.0	](https://gitlab.kitware.com/paraview/paraview/-/issues/19796)
* [CGNS files don't read point data	](https://gitlab.kitware.com/paraview/paraview/-/issues/19838)
* [ExtractBlock does not Propagate Global Field Data	](https://gitlab.kitware.com/paraview/paraview/-/issues/19840)
* [default Piece Distribution method	](https://gitlab.kitware.com/paraview/paraview/-/issues/19857)
* [Glyph filter sometimes produces misplaced arrows	](https://gitlab.kitware.com/paraview/paraview/-/issues/19880)
* [Cell size filter dies with CTH AMR	](https://gitlab.kitware.com/paraview/paraview/-/issues/19908)
* [Intermittent errors in Cell Centers	](https://gitlab.kitware.com/paraview/paraview/-/issues/19921)
* [Glyph filter and stream tracer filter do not propagate Global Field Data	](https://gitlab.kitware.com/paraview/paraview/-/issues/19925)
* [AggregateDataSet filter runs into MPI size constraint	](https://gitlab.kitware.com/paraview/paraview/-/issues/19937)
* [ParaView 5.8 crashes when slicing nested multi-block in parallel	](https://gitlab.kitware.com/paraview/paraview/-/issues/19963)

## Rendering and image export

* [Color legend always shows up in bottom left corner when exporting to EPS or PDF	](https://gitlab.kitware.com/paraview/paraview/-/issues/17639)
* [Low resolution vector graphics output	](https://gitlab.kitware.com/paraview/paraview/-/issues/17846)
* [Preview mode and window resize	](https://gitlab.kitware.com/paraview/paraview/-/issues/17855)
* [Color Map Mapping Data histogram gets confused	](https://gitlab.kitware.com/paraview/paraview/-/issues/19707)
* [OpenGL warning when using Zoom to Box	](https://gitlab.kitware.com/paraview/paraview/-/issues/19718)
* [Surface with Edges creates extra edges	](https://gitlab.kitware.com/paraview/paraview/-/issues/19723)
* [SaveStateAndScreenshot Plugin	](https://gitlab.kitware.com/paraview/paraview/-/issues/19848)
* [Create 1280 X 720 preview	](https://gitlab.kitware.com/paraview/paraview/-/issues/19914)
* [Preview size not preserved in Save Screenshot for multiple views	](https://gitlab.kitware.com/paraview/paraview/-/issues/19919)
* [Background color should be preserved and set by pvsm infrastructure	](https://gitlab.kitware.com/paraview/paraview/-/issues/19958)
* [ParaView Preview Resolution not set on PVSM reload	](https://gitlab.kitware.com/paraview/paraview/-/issues/19959)
* [Preview mode does not rescale text correctly	](https://gitlab.kitware.com/paraview/paraview/-/issues/20024)

## Catalyst

* [File output from Catalyst kills ParaView	](https://gitlab.kitware.com/paraview/paraview/-/issues/19850)
* [Export Inspector, Image Extract, Cinema Image database Advanced Options Window incorrectly named	](https://gitlab.kitware.com/paraview/paraview/-/issues/19793)
* [Annotate Global Data filter does not work with HyperTree Grids	](https://gitlab.kitware.com/paraview/paraview/-/issues/19809)
* [Export Inspector: allow selecting of root directory for pvserver configs	](https://gitlab.kitware.com/paraview/paraview/-/issues/19916)
* [writer for vtkNonOverlappingAMR in Catalyst	](https://gitlab.kitware.com/paraview/paraview/-/issues/20057)

## User interface and interaction

* [[5.6 rc2] Reset Session reset all customized MACROS shortcuts	](https://gitlab.kitware.com/paraview/paraview/-/issues/18528)
* [Preferences -> Camera -> 2D & 3D Mouse Wheel Factor Slider Widgets Inoperable	](https://gitlab.kitware.com/paraview/paraview/-/issues/18545)
* [Enhancement Request: Native 3D Connexion Support on macOS	](https://gitlab.kitware.com/paraview/paraview/-/issues/18704)
* [AnimationView mode has incorrect behavior when loading a dataset with timesteps	](https://gitlab.kitware.com/paraview/paraview/-/issues/19001)
* [Export Inspector: bad Qt layout	](https://gitlab.kitware.com/paraview/paraview/-/issues/19097)
* [Add back PARAVIEW_AUTOLOAD_PLUGIN_XXX or similar	](https://gitlab.kitware.com/paraview/paraview/-/issues/19276)
* [Paraview 5.8.0 UI elements are too large	](https://gitlab.kitware.com/paraview/paraview/-/issues/19751)
* [Grow and Shrink Selection does not work with Find Data	](https://gitlab.kitware.com/paraview/paraview/-/issues/19784)
* [text location not shown correctly in GUI	](https://gitlab.kitware.com/paraview/paraview/-/issues/19798)
* [Crash on View -> Output Messages during reverse connection	](https://gitlab.kitware.com/paraview/paraview/-/issues/19810)
* [error message: Unexpected call to selectionChanged.	](https://gitlab.kitware.com/paraview/paraview/-/issues/19820)
* [Find data multiple expressions error	](https://gitlab.kitware.com/paraview/paraview/-/issues/19821)
* [Find data: Subsetting query causes "ValueError: setting an array element with a sequence."	](https://gitlab.kitware.com/paraview/paraview/-/issues/19822)
* [Keep "Waiting for Server Connection" panel on top of GUI when waiting for connection	](https://gitlab.kitware.com/paraview/paraview/-/issues/19833)
* [Find Data breaks with multiple expressions on multiblock data	](https://gitlab.kitware.com/paraview/paraview/-/issues/19845)
* [PolyLine source points can't be changed manually in the GUI	](https://gitlab.kitware.com/paraview/paraview/-/issues/19918)
* [Icons for Plot Global Variables Over Time and Plot Selection Over Time are flipped	](https://gitlab.kitware.com/paraview/paraview/-/issues/20128)

## Build and installation

* [macOS Catalina: ParaView-5.7.0-MPI-OSX10.12-Python2.7-64bit.pkg” can’t be opened because Apple cannot check it for malicious software.	](https://gitlab.kitware.com/paraview/paraview/-/issues/19432)
* [ParaView build fails on arm clusters	](https://gitlab.kitware.com/paraview/paraview/-/issues/19679)
* [Catalyst: Node class from vtkPlotEdges.cxx name-collides with linked simulations (SPARC)	](https://gitlab.kitware.com/paraview/paraview/-/issues/19695)
* [Follow-up from "Fix paraview-config script"	](https://gitlab.kitware.com/paraview/paraview/-/issues/19730)
* [Spack ParaView should contain  '-DVTK_ENABLE_KITS:BOOL=ON'	](https://gitlab.kitware.com/paraview/paraview/-/issues/19745)
* [Runtime crash on Cray, Debug builds	](https://gitlab.kitware.com/paraview/paraview/-/issues/19775)
* [paraview build fails first time through due to cinema-related build order issue on stria/ARM	](https://gitlab.kitware.com/paraview/paraview/-/issues/19801)
* [Remove unnecessary Python 2 conflict in ParaView's Spack file	](https://gitlab.kitware.com/paraview/paraview/-/issues/19981)
* [Spack: ParaView installs libraries to prefix/lib64 but creates empty prefix/lib making ParaView's modulefile invalid	](https://gitlab.kitware.com/paraview/paraview/-/issues/19999)

## Third-party library updates

* [Upgrade ParaView CGNS version to 4.0.0	](https://gitlab.kitware.com/paraview/paraview/-/issues/19701)


## Miscellaneous bug fixes

* [Segfault calling VisRTX_GetContext() at startup	](https://gitlab.kitware.com/paraview/paraview/-/issues/19863)
