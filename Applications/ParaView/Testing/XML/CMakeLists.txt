# TODO: Split these up and put them in the conditional which actually requires
# the data.
ExternalData_Expand_Arguments(ParaViewData _
  "DATA{${paraview_test_data_directory_input}/Data/,REGEX:tensors_.*\\.vti}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00000/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00000/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00000/particles/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00000/particles/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00005/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00005/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00005/particles/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/AMReX-MFIX/plt00005/particles/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/3GQP.pdb}"
  "DATA{${paraview_test_data_directory_input}/Data/can-restarts/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/CompositeGlyphInput.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/CompositeGlyphTree.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/CompositeGlyphTree/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/CubeStringArray.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/EnSight/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/ExRestarts/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FieldDataBlocks/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FieldDataBlocks/FieldDataBlocks/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FileSeries/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/GenericIOReader/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Iron_Xdmf/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/LoadState.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/LoadStateDialog.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiBlockWithPieces.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiBlockWithPieces_0_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiBlockWithPieces_1_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/NE2_ps_bath.png}"
  "DATA{${paraview_test_data_directory_input}/Data/PropertyLink.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/RectGrid2.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/RodPlate/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/SLAC/pic-example/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/SPCTH/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/SPCTH/Dave_Karelitz_Small/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/SPCTH/DerivedDensity/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Scenario1_p1.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/Scenario1_p1.xmf}"
  "DATA{${paraview_test_data_directory_input}/Data/Test1.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/Test1.xmf}"
  "DATA{${paraview_test_data_directory_input}/Data/TestCompositePropertyWidgetDecoratorPlugin.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/TestMultipleNumberOfComponentsPlugin.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/VisItBridge/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/amr/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/artifact.dta}"
  "DATA{${paraview_test_data_directory_input}/Data/blow.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/blow_data.myvtk}"
  "DATA{${paraview_test_data_directory_input}/Data/bluntfin.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/can.ex2}"
  "DATA{${paraview_test_data_directory_input}/Data/cgns_np01/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cgns_np04/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/comb.q}"
  "DATA{${paraview_test_data_directory_input}/Data/combxyz.bin}"
  "DATA{${paraview_test_data_directory_input}/Data/cow.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/cth.vtr}"
  "DATA{${paraview_test_data_directory_input}/Data/cube.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/cylinder_run1.e}"
  "DATA{${paraview_test_data_directory_input}/Data/cylinder_run2.e}"
  "DATA{${paraview_test_data_directory_input}/Data/disk_out_ref.ex2}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation.clone.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation4.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/elements.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/ensemble-wavelet/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/filters.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/iron_protein.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/linesPolydata.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/manyTypes.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_0.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_1.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_2.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/non_convex_polygon.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/office.binary.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/pointSet.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/porphyrin.cml}"
  "DATA{${paraview_test_data_directory_input}/Data/quadraticTetra01.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/quadratic_tets_with_ghost_cells.pvtu}"
  "DATA{${paraview_test_data_directory_input}/Data/quadratic_tets_with_ghost_cells_0.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/quadratic_tets_with_ghost_cells_1.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/sample.h5part}"
  "DATA{${paraview_test_data_directory_input}/Data/servers-wiki.pvsc}"
  "DATA{${paraview_test_data_directory_input}/Data/sineWaves.csv}"
  "DATA{${paraview_test_data_directory_input}/Data/singleSphereAnimation.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/singleSphereAnimation/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/tensors.pvti}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartdscol.vtpc}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartdscol/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartds.vtpd}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartds/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/timestep_0_15.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/tube.exo}"
  "DATA{${paraview_test_data_directory_input}/Data/vehicle_data.csv}"
  "DATA{${paraview_test_data_directory_input}/Data/viscoplastic-ring.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/waveletElevation.vti}"
  "DATA{${paraview_test_data_directory_input}/Data/waveletMaterial.vti}"

  # Baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AAXAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AAYAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AAZAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid7.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid8.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid9.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BatchAxesGrid.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BoxWidget-Transform.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BoxWidget-ClipDefault.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BoxWidget-ClipCheckUseReferenceBounds.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BoxWidget-ClipUncheckUseReferenceBounds.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CalculatorQuotedVariable1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CalculatorQuotedVariable2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CameraLink-Parallel.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CameraLink-Parallel_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CameraLink-Perspective.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CameraLink-Perspective_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CategoricalAutomaticAnnotationsInterA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CategoricalAutomaticAnnotationsInterB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CategoricalAutomaticAnnotationsInterC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CategoricalAutomaticAnnotationsInterD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsE.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsF.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsG.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsH.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesA_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ClampAndUpdateColorMap-TS12-Clamped.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ClampAndUpdateColorMap-TS12-Unchanged.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ClampAndUpdateColorMap-TS6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorBy.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorLegendScaledScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ConnectivityCountAscending.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ConnectivityCountDescending.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ConvertToMoleculeBonds.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomViewpoints.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomViewpoints_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomViewpoints_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DataAxesGrid2X.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DataAxesGrid-Custom.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DataAxesGrid-CustomWithReset.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnsembleT0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnsembleT1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnsembleT3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExodusModeShapes.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FileSeries1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FileSeries2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog10.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog11.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog12.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog13.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog14.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog15.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog7.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog8.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialog9.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataSelectLocation.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataSelectLocationMultiblock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FreezeQueryMultiblock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ForceTimeDiamond.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Glyph3DRepresentation_Cylinder.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Glyph3DRepresentation_Cylinder_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GlyphWithEmptyCells_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GlyphWithEmptyCells_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GlyphWithEmptyCells_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection2_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HistogramSelection4_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisClip1-Plane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisClip2-RevPlane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisClip3-Box.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisCut.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisReflection.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridCellCenters.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridDepthLimiter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridPlaneCutter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridThreshold-DepthOne.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridThreshold-DepthTwo.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ImportCinema.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ImportCinemaSpecA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/IndexedLookupInitialization.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/IndexedLookupInitialization_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LightAddRemove1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LightAddRemove2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection2_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection4_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LoadStateMultiView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMap7.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMapToggle1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogColorMapToggle2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MergePointBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Molecule-BallStick.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Molecule-Custom.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Molecule-Liquorice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision0Display.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision0Display_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision1Display.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision1Display_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision2Display.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision2Display_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision3Display.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision3Display_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision4Display.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NonlinearSubdivision4Display_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OBBMajorAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OBBMediumAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OBBMinorAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSet_Slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSet_Surface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Surface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PLYWriter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PointGaussianScaleOpacityArrayA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PointGaussianScaleOpacityArrayA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PolyLineSourceInterA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Preview-SingleView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Preview-AllViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PythonAlgorithmPlugin.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PythonAlgorithmReadersAndWriters.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/QuartilePlotArea.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/QuartilePlotArea_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/QuartilePlotLines.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_BCOnly.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_CellData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_FamilySelection.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_PointData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReloadExodusFile-AfterReload.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReloadExodusFile-BeforeReload.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingSurface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingSurface_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingSurface_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingVolume.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderNanDefaultColor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderNanGUIColor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderNanPresetColor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleWithDataset_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleWithDataset_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleWithDataset_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetSession.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeC_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeC_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResetToVisibleRangeE.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreArrayDefaultTransferFunctionA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreArrayDefaultTransferFunctionB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreArrayDefaultTransferFunctionC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreArrayDefaultTransferFunctionC_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreDefaultTransferFunctionA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreDefaultTransferFunctionB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RestoreDefaultTransferFunctionC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReverseLegend_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReverseLegend_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerB_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerC_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerD_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveLargeScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveLargeScreenshot_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveSmallScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveSmallScreenshot_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparatedColorMapA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparatedColorMapB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceDirectDoubleColoring.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlaneCellData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlaneMultiBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlaneMultiBlockSurface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlaneMultiBlockSurface_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SpherePointSource_InterA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SpherePointSource_InterB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SplitViewTraceScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/StepColorSpaceA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/StructuredGridCellBlanking.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/StructuredGridVolumeRendering.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCAVE-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCAVE-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestClipCylinder.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestColorHistogram.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCompositedGeometryCulling.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFindDataMax.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestGeometryBoundsClobber.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestIsoVolume1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestIsoVolume2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestIsoVolume3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestIsoVolume4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestIsoVolume5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestLineChartLabelPrefix.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestMultiServer1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestMultiServer2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestMultiServer3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestMultiServer4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonView_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonViewScript.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Threshold.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdInvert.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-1x1-tile0_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-1x1-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x1-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x1-tile1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x1-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile2_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TimeStepProgressBar_Inter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceExodus.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceMultiViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceTimeControls.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunctionResetOnVisibilityChange_AllBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunctionResetOnVisibilityChange_OneBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunctionResetOnVisibilityChange_TwoBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UniformInverseTransformSamplingGlyph_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UniformInverseTransformSamplingGlyph_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UniformInverseTransformSamplingGlyph_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UpdateTransferFunctionRanges1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UpdateTransferFunctionRanges2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UpdateTransferFunctionRanges3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UpdateTransferFunctionRanges4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UpdateTransferFunctionRanges5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UpdateTransferFunctionRanges6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions1_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions2_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions2_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UseDataPartitions4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeCellSelection.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeNoMapScalars.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_ChangeParameters.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_Default.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_DisableBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_EnableBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_Default.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_Default_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_UseIndexForX.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_UseIndexForX_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResult.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResult2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResultChanged.png}")

SET (TESTS_WITHOUT_BASELINES
  AboutDialog.xml
  ArrayBoundsScaleDomain.xml
  FavoritesFilters.xml
  CustomOutputFormat.xml
  DelimitedTextReader.xml
  DynamicFieldDataDomain.xml
  ExportFilteredColumnsSpreadsheet.xml
  FindWidget.xml
  ModifyGUIWindowTitle.xml
  OpenHelp.xml
  PartialArrayInLineChart.xml
  PointCellDataConversion.xml
  ReloadCGNSFile.xml
  SearchBox.xml
  ServerConnectDialog.xml
  SpreadSheetSelectedCellConnectivity.xml
  StringInLineChartView.xml
  TableHistogram.xml
  TestCompositePropertyWidgetDecorator.xml
)

if (PARAVIEW_PLUGIN_ENABLE_EULATestPlugin)
  list(APPEND TESTS_WITHOUT_BASELINES
    PluginEULA.xml)
endif ()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND TESTS_WITHOUT_BASELINES
    TestPythonConsole.xml
    TestPopOutWidget.xml # needs programmable filter
    MultipleNumberOfComponents.xml
    SpreadSheetNullArrayName.xml # needs programmable filter
  )
endif()

SET (TESTS_WITH_INLINE_COMPARES
  AxesGrid.xml
  BoundingRuler.xml
  BoxWidget.xml
  CalculatorQuotedVariable.xml
  CameraLink.xml
  ChartLoadNoVariables.xml
  ClampAndUpdateColorMap.xml
  Connectivity.xml
  Contour.xml
  DataAxesGrid2X.xml
  Ensemble.xml
  FastUniformGridSource.xml
  FileSeries.xml
  #HyperTreeGridAxisClip.xml
  #HyperTreeGridAxisCut.xml
  #HyperTreeGridAxisReflection.xml
  #HyperTreeGridCellCenters.xml
  #HyperTreeGridContour.xml
  #HyperTreeGridPlaneCutter.xml
  LightAddRemove.xml
  LineChartSelection.xml
  LoadStateMultiView.xml
  LogColorMap.xml
  LogColorMapToggle.xml
  MemoryInspectorPanel.xml
  Molecule.xml
  NonlinearSubdivisionDisplay.xml
  PartitionedDataSet.xml
  PartitionedDataSetCollection.xml
  Preview.xml
  QuartilePlot.xml
  ReadPartitionedCGNS.xml
  RectilinearFractal.xml
  ReloadExodusFile.xml
  RemoteRendering.xml
  RenderNan.xml
  ResetToVisibleRange.xml
  Ruler.xml
  SaveLargeScreenshot.xml
  Slice.xml
  TestIsoVolume.xml
  Threshold.xml
  TransferFunctionResetOnVisibilityChange.xml
  UpdateTransferFunctionRanges.xml
  XYChart.xml
  XYHistogram.xml
)

# These tests do not work in parallel due to issues with
# partitioned vtkHyperTreeGrids. When those issues are fixed
# these tests should be enabled in parallel again by adding
# them back to TESTS_WITH_INLINE_COMPARES
SET (HTG_TESTS_WITH_INLINE_COMPARES
  HyperTreeGridDepthLimiter.xml
  HyperTreeGridThreshold.xml
)
paraview_add_client_tests(
  TEST_SCRIPTS ${HTG_TESTS_WITH_INLINE_COMPARES}
)

if(NOT APPLE)
  # Workaround to avoid testing failures on high-res Retina displays on Macs
  list(APPEND TESTS_WITH_INLINE_COMPARES
    ColorLegendScaledScreenshot.xml
  )
endif()

SET(TESTS_WITH_BASELINES
  DataAxesGrid.xml
  MoleculeScalarBar.xml
  TestHiddenLineRemoval.xml
  TestOpacityRendering.xml
)

if(PARAVIEW_ENABLE_VISITBRIDGE)
  list(APPEND TESTS_WITH_BASELINES
    VisItBridgeChombo.xml
    VisItBridgeLAMMPSDump.xml
    VisItBridgeLAMMPSDump2.xml
    VisItBridgeNas.xml
    VisItBridgeNek.xml
    VisItBridgeMoleculeColoring.xml
    VisItBridgePdb.xml
    VisItBridgePFLOTRAN.xml
    VisItBridgePixie.xml
    VisItBridgeSamrai.xml
    TruchasReaderWithVisItBridge.xml)
else()
  # A number of VisIt readers support reading of .pdb and
  # of .h5 files, so when the VisItBridge is enabled,
  # the test must take extra steps to select the correct reader.
  #
  # When the VisItBridge is not enabled, there is no need to select
  # a reader.
  list(APPEND TESTS_WITH_BASELINES
    MoleculeColoring.xml
    TruchasReader.xml)
endif()

if(PARAVIEW_USE_RAYTRACING)
  list(APPEND TESTS_WITH_BASELINES
    OSPRay.xml)
endif()

if(PARAVIEW_ENABLE_OPENTURNS)
  list(APPEND TESTS_WITH_BASELINES
    HistogramKernelSmoothing.xml)
  list(APPEND TESTS_WITH_BASELINES
    PlotMatrixViewDensityMaps.xml)
endif()

# hits OSX bug with FileDialog so exclude test on APPLE
if (NOT APPLE)
  list (APPEND TESTS_WITH_INLINE_COMPARES
    HistogramSelection.xml)
endif()

# TODO: Port to the new API.
macro(ADD_CATALYST_LIVE_TEST test_name duration command_line_connect_command)
  # These tests are too complex to run in parallel.
  set (CATALYST_SIMULATION CatalystWaveletDriver.py)
  set (INSITU_SCRIPT CatalystWaveletCoprocessing)

  set ("${test_name}_FORCE_SERIAL" TRUE)
  add_pv_test("pv" "_DISABLE_C"
    COMMAND
    --script-ignore-output-errors --script $<TARGET_FILE:ParaView::pvbatch> -sym
    ${CMAKE_CURRENT_SOURCE_DIR}/${CATALYST_SIMULATION}
    ${INSITU_SCRIPT} "${duration}"

    --client ${CLIENT_EXECUTABLE}
    --enable-bt
    ${command_line_connect_command}
    -dr
    --test-directory=${PARAVIEW_TEST_DIR}
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${test_name}.xml
    EXTRA_LABELS "catalyst")

  add_pv_test(pvcs "_DISABLE_CS"
    COMMAND
    --script-np 5 --script-ignore-output-errors
    --script $<TARGET_FILE:ParaView::pvbatch> -sym
    ${CMAKE_CURRENT_SOURCE_DIR}/${CATALYST_SIMULATION}
    ${INSITU_SCRIPT} "${duration}"

    --server $<TARGET_FILE:ParaView::pvserver>
    --enable-bt

    --client ${CLIENT_EXECUTABLE}
    --enable-bt
    ${command_line_connect_command}
    -dr
    --test-directory=${PARAVIEW_TEST_DIR}
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${test_name}.xml
    EXTRA_LABELS "catalyst")

endmacro(ADD_CATALYST_LIVE_TEST)

# Catalyst tests
# ----------------------------------------------------------------------
if(PARAVIEW_ENABLE_CATALYST AND PARAVIEW_ENABLE_PYTHON)

  paraview_add_client_tests(
    FORCE_SERIAL
    TEST_SCRIPTS CatalystGUI.xml )

  if (TARGET ParaView::CinemaPython)
    paraview_add_client_tests(
      FORCE_SERIAL
      TEST_SCRIPTS CatalystCinemaGUI.xml )
  endif ()

  #add_catalyst_live_test(CatalystLiveSetBreakpoint 222 "")
  # the CatalystLivePause test uses the --live=22222 command line option to automatically
  # connect to a running simulation on port 22222
  #add_catalyst_live_test(CatalystLivePause 80  "--live=22222")

  # tests for the new export inspector panel
  paraview_add_client_tests(
    FORCE_SERIAL
    TEST_SCRIPTS CatalystExporterGUI.xml )
  paraview_add_client_tests(
    FORCE_SERIAL
    DATA_DIRECTORY "${paraview_test_data_directory_output}"
    TEST_SCRIPTS TemporalExporterGUI.xml )

endif()

if (PARAVIEW_ENABLE_XDMF2 AND NOT PARAVIEW_ENABLE_XDMF3)
  # Xdmf2 is optional.
  # And when both Xdmf2 and Xdmf3 are enabled, the SelectReader dialog
  # confuses these tests. So, we'll disable them for now.
  # We need a better mechanism to handle such cases in the testing framework.
  list(APPEND TESTS_WITH_BASELINES
    ExtractBlock.xml
    XdmfRead.xml
    XdmfReadImageData.xml
    XdmfReadImageDataCollection.xml
    XdmfReadRectilinearGrid.xml
    XdmfReadRectilinearGridCollection.xml
    XdmfReadStructuredGrid.xml
    XdmfReadStructuredGridCollection.xml
    XdmfGridAttributes.xml
    ZLibXDMF.xml
    )
endif()

list(APPEND TESTS_WITH_BASELINES
  4AxesContextView.xml
  AccentuatedFilename.xml
  AMRCutPlane.xml
  AMReXParticlesReader.xml
  Annotations.xml
  ConvertToMolecule.xml
  CopyPasteProperties.xml
  CTHAMRBaseline.xml
  CTHAMRClip.xml
  CTHAMRContour.xml
  CTHAMRDualClip.xml
  CTHAMRMaterialInterfaceFilter.xml
  CTHDerivedDensity2DClyinder.xml
  AddActiveValuesFromVisibleObjects.xml
  AnimatePipelineTime.xml
  AngularPeriodicFilter.xml
  3DWidgetInCustomFilter.xml
  CalcParens.xml
  Calculator.xml
  CalculatorInput.xml
  ChangeGlyphRepVisibility.xml
  ChangingTimestepsInStateFiles.xml
  ChartAxisRangeAndLabels.xml
  CheckableHeader.xml
  Clip.xml
  ColorEditorControls.xml
  ColorEditorVolumeControls.xml
  ColorLegendMinMaxLabels.xml
  ColorOpacityTableEditing.xml
  CategoricalAutomaticAnnotations.xml
  CategoricalColors.xml
  CategoricalOpacities.xml
  ColorByCellDataStringArray.xml
  ComparativeVisPanel.xml
  ComparativeViewOverlay.xml
  # disabling overlay test for now. It has issues with Time 0.
  #  ComparativeOverlay.xml
  CompositeSurfaceSelection.xml
  CreateDelete.xml
  CustomFilter.xml
  CustomSourceProbe.xml
  CustomViewpoints.xml
  DataOutlines.xml
  DisableResetDisplayEmptyViews.xml
  DualSphereAnimation.xml
  EnSight.xml
  EnSightTensorInversion.xml
  EnSightTensorInversionBin.xml
  ExodusXML.xml
  ExodusGlobalDataWithRestarts.xml
  ExportLinePlotToCSV.xml
  ExportLinePlotToTSV.xml
  ExportSceneSpreadSheetView2.xml
  ExportX3dPOVVRML.xml
  ExTimeseries.xml
  ExTimeseries2.xml
  ExtractComponentFilter.xml
  ExtractLevel.xml
  ExtractTimeSteps.xml
  FeatureEdges.xml
  FFTOverTime.xml
  FieldDataBlocks.xml
  Flagpole.xml
  Flow.xml
  Flow2.xml
  FollowHiddenData.xml
  ForceTime.xml
  Fractal2D.xml
  Glyph3DCullingAndLOD.xml
  Glyph3DRepresentation.xml
  GlyphUseCellCenters.xml
  GlyphWithEmptyCells.xml
  H5PartReader.xml
  HideAll.xml
  HorizontalColorLegendTitle.xml
  IgnoreLogAxisWarning.xml
  ImageVolumeRendering.xml
  InteractiveSelection.xml
  InteractiveViewLinkOffscreen.xml
  InteractiveViewLinkOnscreen.xml
  InteractiveViewLinkState.xml
  LinesAsTubes.xml
  LoadPartiallyUpdatedStateFiles.xml
  LoadSaveStateAnimation.xml
  LoadState.xml
  LoadStateChooseDirectory.xml
  LoadStateChooseFiles.xml
  LoadStateUseExisting.xml
  LoadSaveStateVolume.xml
  ManyTypesXMLWriterReader.xml
  MergePointBlocks.xml
  MoleculeBonds.xml
  MoleculeToLines.xml
  MultiBlockAttributes1.xml
  MultiBlockInspector.xml
  MultiBlockSingleBlock.xml
  MultiSliceMultiBlock.xml
  MultiSliceWavelet.xml
  NormalGlyphs.xml
  NumberOfComponentsDomain.xml
  NonConvexPolygon.xml
  OpenSaveData.xml
  OrderedGroupDatasets.xml
  OrthographicView.xml
  OutOfRangeColors.xml
  PlotDataOverTime-NonDistributed.xml
  PlotEdges2.xml
  PlotEdges.xml
  PlotGlobalVariablesOverTime.xml
  PlotOverTimeAutoApply.xml
  PolygonCellSelection.xml
  PolygonPointSelection.xml
  PointChartView.xml
  PointGaussianMultiBlockDataSet.xml
  PointGaussianNoScaleTransferFunction.xml
  PointGaussianScaleOpacityArray.xml
  PointInterpolator.xml
  PolarAxes.xml
  PolyLineSource.xml
  PropertyConversion.xml
  PropertyConversion1.xml
  PropertyConversion2.xml
  PropertyConversion3.xml
  PropertyLink.xml
  QuadraticGhostCells.xml
  ReadXMLPolyDataFileSeries.xml
  RepresentationSelector.xml
  ResampleWithDataset.xml
  RescaleToTemporal.xml
  ReverseLegend.xml
  RotateCamera90.xml
#  SaveAnimationGeometry.xml
  SaveColorMap.xml
  SaveCSV.xml
  SaveOBJ.xml
  SaveTSV.xml
  SaveTXT.xml
  SelectedProxyPanelVisibility.xml
  SelectionModifiersCells.xml
  SelectionModifiersPoints.xml
  SelectionModifiersBlocks.xml
  SetCustomBasicPresets.xml
  ShaderReplacements.xml
  ShowAll.xml
  SimpleInteraction.xml
  SliceAlongPolyline.xml
  SliceAlongSpline.xml
  SolidColorSource.xml
  SphereSlice.xml
  SpherePointSource.xml
  SpreadSheet1.xml
  SpreadSheet2.xml
  SpreadSheet3.xml
  SPTimeseries.xml
  SpyPlotHistoryReader.xml
  SelectionLinkBasic.xml
  SelectionLinkConverted.xml
  SelectionLinkHistogram.xml
  SelectionLinkInitial.xml
  SelectionLinkMultiple.xml
  SelectionLinkRemove.xml
  SelectionLinkReaction.xml
  SelectionLinkReactionNonConverted.xml
  SeparatedColorMap.xml
  SeparatedColorMapOpacity.xml
  SliceWithPlane.xml
  SliceWithPlaneMultiBlock.xml
  SortLineChartData.xml
  ScrollAreaNativeWidget.xml
  StepColorSpace.xml
  StockColors.xml
  StreamTracerSurface.xml
  StreamTracerUpdates.xml
  TabbedViews.xml
  TensorGlyph.xml
  TemporalInterpolator.xml
  TemporalShiftScale.xml
  TestGroupDataFromTimeSeries.xml
  TestSelectionOnMultipiece.xml
  TimeInspector.xml
  TimeStepProgressBar.xml
  Tessellate.xml
  UndoRedo.xml
  UndoRedo1.xml
  UndoRedo2.xml
  UndoRedo3.xml
  UndoRedo5.xml
  UndoRedo6.xml
  UndoRedo7.xml
  UndoRedo8.xml
  UnstructuredOutline.xml
  UniformInverseTransformSamplingGlyph.xml
  VectorComponentHistogram.xml
  VolumeRenderingWithContour.xml
  VolumeReprTwoIndepComp.xml
  VariableSelector.xml
  VariableSelector1.xml
  ViewSettingsDialog.xml
  VTPSeriesFile.xml
  #  XYBarChart.xml
  RandomAttributes.xml
)

if (TARGET VTK::IOParallelLSDyna)
  list(APPEND TESTS_WITH_BASELINES
    Plot3DReader.xml
    SelectReader.xml
    )
endif ()

if(VTK_USE_LARGE_DATA)
  ExternalData_Expand_Arguments(ParaViewData _
    "DATA{${paraview_test_data_directory_input}/Data/bake/bake.e}")
  list(APPEND TESTS_WITH_BASELINES
    CheckSelectedBlocks.xml
    RestoreBlockColorDefaultTransferFunction.xml
    )
endif()

# The color dialog is different on MacOS X which makes this test fail.
if(NOT APPLE)
  list(APPEND TESTS_WITH_BASELINES
    BackgroundLights.xml
    )
endif()

list(APPEND TESTS_WITH_BASELINES
  # This test makes use of "Point Gaussian" representation, only available with OpenGL2
  BackgroundColorCheck.xml

  # PlotMatrixView is bugging in OpenGL1
  PlotMatrixViewArraySelection.xml

  # Composite Glyph Trees are only implemented in OpenGL2
  CompositeGlyphTree.xml
  )

if (PARAVIEW_USE_MPI)
  list(APPEND TESTS_WITH_BASELINES
    # Test uses D3 which is available in MPI only builds.
    D3SmallCells.xml

    # Test uses D3 which is available in MPI only builds.
    GridConnectivity.xml

    # Test uses D3 which is available in MPI only builds.
    ExportSelectionToCSV.xml

    # This test check spreadsheet values which include processID
    ManyTypesXMLWriterReaderMPI.xml

    # Test Use Data Partitions for volume rendering
    #UseDataPartitions.xml
  )

  # DistributePoints is only tested in non-built-in mode.
  paraview_add_client_server_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS DistributePoints.xml
  )
  paraview_add_client_server_render_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS DistributePoints.xml
  )

  # Volume rendering seems to work better in serial
  set (UseDataPartitions_FORCE_SERIAL TRUE)
  # This test requires a parallel partition to make sense so we
  # skip running it with the built-in server.
  paraview_add_client_server_tests(
    TEST_SCRIPTS UseDataPartitions.xml
  )


  # Test AppendReduce which can only be tested with non-built-in mode
  paraview_add_client_server_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS AppendReduce.xml
  )
  # Test AppendReduce which can only be tested with non-built-in mode
  paraview_add_client_server_render_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS AppendReduce.xml
  )

  # Test uses GhostCellsGenerator which is available in MPI only builds.
  # As the test use pvtu writer/reader it does not work in built-in mode.
  paraview_add_client_server_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS GhostCellsGenerator.xml
    )
  paraview_add_client_server_render_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS GhostCellsGenerator.xml
    )

  if (PARAVIEW_ENABLE_COSMOTOOLS)
    # Test the Generic IO file writer in VTKExtensions/CosmoTools
    ExternalData_Expand_Arguments(ParaViewData _
      "DATA{${paraview_test_data_directory_input}/Data/genericio/m000.499.allparticles}")
    paraview_add_client_tests(
      ARGS --mpi
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS GenericIOReadWrite.xml
      )
    paraview_add_client_server_tests(
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS GenericIOReadWrite.xml
      )
    paraview_add_client_server_render_tests(
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS GenericIOReadWrite.xml
      )

  endif()
  list(APPEND TESTS_WITH_BASELINES
    CGNSReader-5blocks_cgns.xml)

  # PointInterpolator only works in non-distributed modes.
  set(PointInterpolator_DISABLE_CRS TRUE)
  set(PointInterpolator_DISABLE_CS TRUE)
endif()
if(PARAVIEW_ENABLE_PYTHON)
  list(APPEND TESTS_WITH_BASELINES
    ColorByComponentNames.xml # needs programmable filter
    LiveProgrammableSource.xml
    SaveLoadStatePython.xml
    SelectionLabels.xml
    TraceExodus.xml
    TraceExportAndSaveData.xml
    TraceSaveGeometry.xml
    TraceStatisticsFilter.xml
    TraceSupplementalProxiesFully.xml
    TraceTimeControls.xml
  )
  list(APPEND TESTS_WITH_INLINE_COMPARES
    RestoreArrayDefaultTransferFunction.xml
    RestoreDefaultTransferFunction.xml
    TraceMultiViews.xml
    SplitViewTrace.xml
  )
  list(APPEND TESTS_WITHOUT_BASELINES
    PythonCalculator.xml
    TraceIntegrateVariables.xml
  )

  # Check that matplotlib is available
  include(FindPythonModules)
  find_python_module(matplotlib matplotlib_found)
  if (matplotlib_found)
    list(APPEND TESTS_WITH_BASELINES TestPythonView.xml)
  endif()

  find_python_module(numpy numpy_found)
  if (numpy_found)
    list(APPEND TESTS_WITH_BASELINES
      ExodusModeShapes.xml
      FindDataSelectLocation.xml # find data needs python/numpy
      FindDataSelectLocationMultiblock.xml # find data needs python/numpy
      ForceTimeDiamond.xml
      FreezeQueryMultiblock.xml
      IndexedLookupInitialization.xml # needs Python Calculator/numpy
      StructuredGridCellBlanking.xml
      TestFindDataMax.xml
      VolumeNoMapScalars.xml # needs programmable filter + numpy
      )
    list(APPEND TESTS_WITH_INLINE_COMPARES
      FindDataDialog.xml)

    if (TARGET ParaView::CinemaReader)
      # Cinema importing capabilities are only available in Python + OpenGL2
      # add the cinema database files.
      set(cinema_data
        "cinema-composite.cdb/image/info.json")
      foreach(poseidx RANGE 0 17)
        foreach(visidx RANGE 0 1)
          list(APPEND cinema_data
            "cinema-composite.cdb/image/pose=${poseidx}/vis=${visidx}/,REGEX:.*")
        endforeach()
      endforeach()
      list(APPEND cinema_data
        "cinema-non-composite.cdb/image/info.json")
      foreach(phi 0 90 180)
        list(APPEND cinema_data
          "cinema-non-composite.cdb/image/${phi}/0/,REGEX:.*")
      endforeach()
      set(cinema_data_refs)
      foreach (cinema_data_file IN LISTS cinema_data)
        list(APPEND cinema_data_refs
          "DATA{${paraview_test_data_directory_input}/Data/${cinema_data_file}}")
      endforeach ()
      ExternalData_Expand_Arguments(ParaViewData _
        ${cinema_data_refs})

      list(APPEND TESTS_WITH_BASELINES
        ImportCinema.xml
        ImportCinemaSpecA.xml)

      # presently, we don't support client-server configurations.
      set(ImportCinema_DISABLE_CS TRUE)
      set(ImportCinema_DISABLE_CRS TRUE)
      set(ImportCinemaSpecA_DISABLE_CS TRUE)
      set(ImportCinemaSpecA_DISABLE_CRS TRUE)
    endif ()


    # PythonAlgorithm plugin tests.
    configure_file(
      "PythonAlgorithmPlugin.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/PythonAlgorithmPlugin.xml" @ONLY)
    configure_file(
      "PythonAlgorithmReadersAndWriters.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/PythonAlgorithmReadersAndWriters.xml" @ONLY)

    set(pyalgo_plugin_tests
      PythonAlgorithmPlugin
      PythonAlgorithmReadersAndWriters)
    foreach(tname IN LISTS pyalgo_plugin_tests)
      list(APPEND TESTS_WITH_BASELINES
        ${CMAKE_CURRENT_BINARY_DIR}/${tname}.xml)

      # we need to extend testing infrastructure to better support
      # loading plugins in client-server. At that point, we can test these as well.
      set(${tname}_DISABLE_CS TRUE)
      set(${tname}_DISABLE_CRS TRUE)
    endforeach()
  endif()
endif()

#------------------------------------------------------------------------------
# Add streaming tests.
# We need to locate smooth.flash since it's not included in the default testing
# datasets.
find_file(smooth_flash NAMES smooth.flash
          DOC "Path to smooth.flash data file."
          NO_DEFAULT_PATH)
mark_as_advanced(smooth_flash)
if (EXISTS "${smooth_flash}")
  # we configure the file since we need to point to smooth_flash.
  configure_file("AMRStreaming.xml.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/AMRStreaming.xml" @ONLY)
  configure_file("AMRVolumeRendering.xml.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/AMRVolumeRendering.xml" @ONLY)

  set (streaming_tests
    ${CMAKE_CURRENT_BINARY_DIR}/AMRStreaming.xml)

  # AMRVolumeRendering is a non-streaming test.
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_BINARY_DIR}/AMRVolumeRendering.xml)

  foreach (tname ${streaming_tests})
    add_pv_test("pv" "_DISABLE_C"
      COMMAND --client ${CLIENT_EXECUTABLE}
              --enable-bt
              -dr
              --enable-streaming
              --test-directory=${PARAVIEW_TEST_DIR}

      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${tname})

    add_pv_test(pvcs "_DISABLE_CS"
      COMMAND
         --server $<TARGET_FILE:ParaView::pvserver>
         --enable-bt
         --enable-streaming
         --client ${CLIENT_EXECUTABLE}
         --enable-bt
         -dr
         --enable-streaming
         --test-directory=${PARAVIEW_TEST_DIR}
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${tname})
  endforeach()
endif()

option(PARAVIEW_SSH_SERVERS_TESTING
    "Add SSH Servers testing"
    OFF)
mark_as_advanced(PARAVIEW_SSH_SERVERS_TESTING)

if(PARAVIEW_SSH_SERVERS_TESTING)
  message(STATUS
    "With PARAVIEW_SSH_SERVERS_TESTING, to have the SSH Server tests pass, make sure that this machine has its own ssh public key as an authorized_keys and that 127.0.0.1 is in the known_hosts file, /usr/bin/xterm is available. In case of failing tests, pvserver logs are available in ${CMAKE_CURRENT_BINARY_DIR}")

  configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/server.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tmp/server.sh" @ONLY)
  configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/server_rc.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tmp/server_rc.sh" @ONLY)

  file(
    COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/server.sh
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )

  file(
    COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/server_rc.sh
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )

  configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/sshServers.pvsc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sshServers.pvsc" @ONLY)

  set(ssh_server_tests
      SimpleSSHServer
      SimpleSSHServerAskPass
      SimpleSSHServerTermExec
      SimpleRCSSHServer
      SSHServerPortForwarding
      RCSSHServerPortForwarding
      )

  foreach(tname IN LISTS ssh_server_tests)
    configure_file (
      "${tname}.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${tname}.xml" @ONLY)

    list(APPEND TESTS_WITHOUT_BASELINES
      ${CMAKE_CURRENT_BINARY_DIR}/${tname}.xml)
    set(${tname}_DISABLE_CS TRUE)
    set(${tname}_DISABLE_CRS TRUE)
# The SSH tests should not be run in parallel to avoid port collision
    set(${tname}_FORCE_SERIAL TRUE)
  endforeach()
endif(PARAVIEW_SSH_SERVERS_TESTING)

#------------------------------------------------------------------------------

# Probe picking does not work in render server mode

# Selection is not available in CRS mode
set(MultiSliceWavelet_DISABLE_CRS TRUE)
set(MultiSliceMultiBlock_DISABLE_CRS TRUE)
set(PolygonCellSelection_DISABLE_CRS TRUE)
set(PolygonPointSelection_DISABLE_CRS TRUE)
set(NonlinearSubdivisionDisplay_DISABLE_CRS TRUE)
set(InteractiveSelection_DISABLE_CRS TRUE)
set(NonConvexPolygon_DISABLE_CRS TRUE)
set(QuartilePlot_DISABLE_CRS TRUE)
set(TestSelectionOnMultipiece_DISABLE_CRS TRUE)
set(CheckSelectedBlocks_DISABLE_CRS TRUE)
set(SelectionModifiersCells_DISABLE_CRS TRUE)
set(SelectionModifiersBlocks_DISABLE_CRS TRUE)
set(SelectionModifiersPoints_DISABLE_CRS TRUE)
set(SelectionLinkBasic_DISABLE_CRS TRUE)
set(SelectionLinkInitial_DISABLE_CRS TRUE)
set(SelectionLinkMultiple_DISABLE_CRS TRUE)
set(SelectionLinkRemove_DISABLE_CRS TRUE)
set(SelectionLinkReaction_DISABLE_CRS TRUE)
set(ExportSelectionToCSV_DISABLE_CRS TRUE)

# These tests cannot be run using ctest -j since they are affected by focus
# changed events.
set (AnimatePipelineTime_FORCE_SERIAL TRUE)
set (FavoritesFilters_FORCE_SERIAL TRUE)
set (PolygonCellSelection_FORCE_SERIAL TRUE)
set (PolygonPointSelection_FORCE_SERIAL TRUE)
set (Contour_FORCE_SERIAL TRUE) # since this uses popup-menu
set (Calculator_FORCE_SERIAL TRUE) # since this uses popup-menu
set (LogColorMap_FORCE_SERIAL TRUE) # since this uses popup-menu
set (LogColorMapToggle_FORCE_SERIAL TRUE) # since this pops up output window
set (ColorOpacityTableEditing_FORCE_SERIAL TRUE) # since this uses popup-menu
set (SimpleInteraction_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ExportFilteredColumnsSpreadsheet_FORCE_SERIAL TRUE) # since this uses popup-menu
set (FindWidget_FORCE_SERIAL TRUE) # Uses inline edit popups
set (PropertyLink_FORCE_SERIAL TRUE)
set (TestPythonView_FORCE_SERIAL TRUE) # Seems to work better in serial.
set (SaveColorMap_FORCE_SERIAL TRUE) # Uses inline edit popups
set (CategoricalColors_FORCE_SERIAL TRUE) # Uses inline edit popups
set (ChartAxisRangeAndLabels_FORCE_SERIAL TRUE) # Uses inline edit popups
set (ComparativeViewOverlay_FORCE_SERIAL TRUE) # Seems to works better in serial
set (CategoricalAutomaticAnnotations_FORCE_SERIAL TRUE) # Seems to works better in serial
set (CategoricalOpacities_FORCE_SERIAL TRUE) # Seems to works better in serial

set (LoadSaveStateVolume_FORCE_SERIAL TRUE)
# We don't support volume rendering of image data in data server & render server mode
set(LoadSaveStateVolume_DISABLE_CRS TRUE)
set(SeparatedColorMapOpacity_DISABLE_CRS TRUE)
set(VolumeReprTwoIndepComp_DISABLE_CRS TRUE)

# Set properties for CTH tests
set(CTHAMRContour_DISABLE_CS TRUE)
set(CTHAMRContour_DISABLE_CRS TRUE)
set(CTHAMRDualClip_DISABLE_CS TRUE)
set(CTHAMRDualClip_DISABLE_CRS TRUE)
set(CTHAMRClip_DISABLE_CS TRUE)
set(CTHAMRClip_DISABLE_CRS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CRS TRUE)

# Since this test needs selection, disable in CRS mode.
set (SelectionLabels_DISABLE_CRS TRUE)

# Add image threshold overrides for tests.
# Generally all tests with wireframes need higher thresholds.
SET (Flow2_THRESHOLD 15)
SET (UndoRedo1_THRESHOLD 15)
SET (UndoRedo4_THRESHOLD 15)

# Differences between systems on whether the lines are anti-aliased or not
SET (SortLineChartData_THRESHOLD 50)

# Since often there are issues with fonts etc.
SET (XYChart_THRESHOLD 150)
SET (CheckableHeader_THRESHOLD 150)
# Since the test uses surface-selection, it cannot work in render-server mode.
set(XYChart_DISABLE_CRS TRUE)

SET (XYHistogram_THRESHOLD 150)
set (ChartAxisRangeAndLabels_THRESHOLD 60)

# The color and opacity are often different on different machines.
SET (ColorEditorControls_THRESHOLD 60)
SET (ColorEditorVolumeControls_THRESHOLD 300)

# PlotDataOverTime produces different block labels in parallel
# This leeway helps that.
set (PlotDataOverTime-NonDistributed_THRESHOLD 25)


# Plot image leeway.
set (SpreadSheet2_THRESHOLD 50)
set (PlotOverTimeAutoApply_THRESHOLD 50)
set (ExportLinePlotToCSV_THRESHOLD 30)
set (ExportLinePlotToTSV_THRESHOLD 30)
set (PlotOverTimeAutoApply_DISABLE_CRS TRUE) # since this uses surface selection.
set (QuartilePlot_THRESHOLD 20)
# The ExportLinePlotToTSV test uses the same image baseline as the ExportLinePlotToCSV test
set (ExportLinePlotToTSV_BASELINE ExportLinePlotToCSV.png)

# ExportSceneSpreadSheetView2 won't work in parallel CS because of ghost cells
# than cannot be processed by TableToStructuredGrid.
set(ExportSceneSpreadSheetView2_DISABLE_CS TRUE)
set(ExportSceneSpreadSheetView2_DISABLE_CRS TRUE)

# Cannot support CRS since we do volume rendering of image data
# in this test.
set (RemoteRendering_DISABLE_CRS TRUE)
set (RemoteRendering_THRESHOLD 50)
set (VolumeRenderingWithContour_DISABLE_CRS TRUE)
set (VolumeRenderingWithContour_THRESHOLD 50)
set (VolumeNoMapScalars_DISABLE_CRS TRUE)
set (VolumeNoMapScalars_THRESHOLD 50)


# Histogram filter produces different results when running in parallel, so
# disable it.
IF (PARAVIEW_USE_MPI)
  SET (SpreadSheet1_DISABLE_CS TRUE)
  SET (SpreadSheet1_DISABLE_CRS TRUE)

  # Disabled since the 1 column in spreadsheet view ends up being "Process ID"
  # which messes up the sorting in this test. Need to extend the testing framework
  # or fix spreadsheet view for these to work properly.
  SET (SpreadSheet3_DISABLE_CS TRUE)
  SET (SpreadSheet3_DISABLE_CRS TRUE)

  # The hierchical fractal source is a temporary testing source and it does not
  # create the dataset correctly in parallel. Since it's a testing source, I am
  # just going to disable the test in parallel. We can fix the source when
  # needed.
  SET (RectilinearFractal_DISABLE_CS TRUE)
  SET (RectilinearFractal_DISABLE_CRS TRUE)

  # Selections end up highlighting different set of ID based points in parallel.
  # Hence disable them.
  set (LineChartSelection_DISABLE_CS TRUE)
  set (LineChartSelection_DISABLE_CRS TRUE)

  # Selection link may highlight incorrect ID in parallel,
  # hence disable them.
  set(SelectionLinkBasic_DISABLE_CS TRUE)
  set(SelectionLinkInitial_DISABLE_CS TRUE)
  set(SelectionLinkMultiple_DISABLE_CS TRUE)
  set(SelectionLinkRemove_DISABLE_CS TRUE)
  set(SelectionLinkReaction_DISABLE_CS TRUE)

  if (PARAVIEW_ENABLE_PYTHON)
    set (TraceStatisticsFilter_DISABLE_CS TRUE)
    set (TraceStatisticsFilter_DISABLE_CRS TRUE)
  endif()

  # Many types has a MPI version and Sequential version
  set(ManyTypesXMLWriterReader_DISABLE_CS TRUE)
  set(ManyTypesXMLWriterReader_DISABLE_CRS TRUE)
  set(ManyTypesXMLWriterReaderMPI_DISABLE_C TRUE)
ENDIF ()

# Composite Surface Selection is currently broken in everything but builtin
SET (CompositeSurfaceSelection_DISABLE_CS TRUE)
SET (CompositeSurfaceSelection_DISABLE_CRS TRUE)

# Clip test has object picking which is not supported in client-render-server
# mode.
SET (Clip_DISABLE_CRS TRUE)

# Image volume rendering not supported in CRS mode.
set (ImageVolumeRendering_DISABLE_CRS TRUE)

# These Xdmf tests have wireframes, hence the increased thresholds.
SET (XdmfReadImageData_THRESHOLD 20)
SET (XdmfReadRectilinearGrid_THRESHOLD 20)
SET (XdmfReadRectilinearGridCollection_THRESHOLD 20)
SET (XdmfReadStructuredGrid_THRESHOLD 20)
SET (XdmfReadStructuredGridCollection_THRESHOLD 20)
SET (XdmfReadImageDataCollection_THRESHOLD 20)

# This test renders points and they are offsetted a bit differently on
# different platforms so just increase the threshold
SET (SaveCSV_THRESHOLD 40)
SET (SaveTSV_THRESHOLD 40)
SET (SaveTXT_THRESHOLD 40)
# The SaveTXT and SaveTSV tests use the same image baseline as the SaveCSV test
SET (SaveTSV_BASELINE SaveCSV.png)
SET (SaveTXT_BASELINE SaveCSV.png)

# Disable some testing configurations for these tests.

## Disable ClientRenderServer tests for FFTOverTime. This is done since
## selection is not supported in render server mode esp. when number of render
## server processes is not the same as the data server processes
set (FFTOverTime_DISABLE_CRS TRUE)
set (FFTOverTime_THRESHOLD 70) # since there's a plot, account for minor
                               # rendering differences.

# ColorEditorVolumeControls does volume rendering of structrued data which required remote
# rendering in client-serve mode.
SET (ColorEditorVolumeControls_DISABLE_CS TRUE)
SET (ColorEditorVolumeControls_DISABLE_CRS TRUE)

# ViewSettingsDialog uses texture background which is currently supported only
# in local render mode.
SET (ViewSettingsDialog_DISABLE_CS TRUE)
SET (ViewSettingsDialog_DISABLE_CRS TRUE)

# MultiBlockAttributes1 requires selection which doesn't work on pvcrs
SET (MultiBlockAttributes1_DISABLE_CRS TRUE)

# Plugins are only built as shared libraries.
IF (NOT BUILD_SHARED_LIBS)
  SET (NiftiReaderWriterPlugin_DISABLE_C TRUE)
ENDIF ()
# There should be a client server specific version of this test.
SET (NiftiReaderWriterPlugin_DISABLE_CS TRUE)
SET (NiftiReaderWriterPlugin_DISABLE_CRS TRUE)

# Set image threshold overrides for the tests.
SET(SaveLargeScreenshot_THRESHOLD 40)

# Make these tests use reverse connection.
SET (CutMulti_REVERSE_CONNECT TRUE)

# Since this involves charts.
SET(HistogramSelection_THRESHOLD 40)
SET(LineChartSelection_THRESHOLD 40)

# Increate threshold for this one since the chart axes
# tends to render slightly differently.
set (LoadStateMultiView_THRESHOLD 20)

# Selection not supported in CRS and is needed for this test.
set (ResetToVisibleRange_DISABLE_CRS TRUE)

# Molecules are not supported in client-serve modes, currently.
#set (Molecule_DISABLE_CS TRUE)
#set (Molecule_DISABLE_CRS TRUE)
#set (MoleculeColoring_DISABLE_CS TRUE)
#set (MoleculeColoring_DISABLE_CRS TRUE)

##########################################################
# List of known incompatible tests for Collaboration
#
#   Contains Undo/Redo actions:
#     - Clip, UndoRedo, UndoRedo[1-8]
#
#   Contains Plugin Loading actions:
#     - LoadPlugins, NiftiReaderWriterPlugin
#
#   Does not work in CS or CRS mode
#     - ViewSettingsDialog, RectilinearFractal, NewColorEditor
#
#   Does not work because a disconnection is involved
#     - ExportX3dPOVVRML
#
#   Don't want to support for now
#     - ComparativeVisPanel
#     - LoadSaveStateAnimation: The time get reset by collab
#
#   Timing issues (Works on speedy computer)
#     - 3DWidgetInCustomFilter, CustomFilter
#
##########################################################

OPTION(PARAVIEW_MULTI_CLIENT_TESTING
    "Add Collaboration testing"
    ON)
MARK_AS_ADVANCED(PARAVIEW_MULTI_CLIENT_TESTING)

# The create/delete test was only meant for collaboration
SET (CreateDelete_DISABLE_C TRUE)
SET (CreateDelete_DISABLE_CS TRUE)
SET (CreateDelete_DISABLE_CRS TRUE)

# List of test Enable for collaboration
#set (AnimatePipelineTime_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (CalcParens_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Calculator_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (CameraLink_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (CompositeSurfaceSelection_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (Contour_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ContourRange_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (CreateDelete_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (CustomSourceProbe_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (D3SmallCells_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (DualSphereAnimation_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (EnSight_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ExodusModeShapes_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ExodusXML_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ExTimeseries_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ExTimeseries2_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ExtractBlock_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ExtractLevel_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (FindDataDialog_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (FFTOverTime_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Flow_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Flow2_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Fractal2D_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (GridConnectivity_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (NormalGlyphs_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (OpenSaveData_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Plot3DReader_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (PlotEdges_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (PlotEdges2_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (PropertyConversion_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (PropertyConversion1_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (PropertyConversion2_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
## disabling since the state file doesn't have view layout and such old state
## files are not currently supported in collaborative mode.
##set (PropertyLink_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (ReadXMLPolyDataFileSeries_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (RepresentationSelector_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SaveColorMap_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SaveCSV_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SelectReader_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SimpleInteraction_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Slice_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (SpreadSheet1_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SpreadSheet2_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SPTimeseries_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (SpyPlotHistoryReader_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (StreamTracerUpdates_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (TemporalInterpolator_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (TemporalShiftScale_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Tessellate_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (Threshold_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (UnstructuredOutline_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (VariableSelector_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (VariableSelector1_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (XdmfRead_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfReadImageData_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfReadImageDataCollection_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfReadRectilinearGrid_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfReadRectilinearGridCollection_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfReadStructuredGrid_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfReadStructuredGridCollection_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
#set (XdmfGridAttributes_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})
set (ZLibXDMF_ENABLE_MULTI_CLIENT ${PARAVIEW_MULTI_CLIENT_TESTING})


set(TESTS_WITH_MULTI_SERVERS_3
  TestMultiServer3.xml
)

list(APPEND TESTS_WITHOUT_BASELINES
  ${TESTS_WITH_INLINE_COMPARES})

paraview_add_client_tests(
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES}
)

paraview_add_client_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES}
)

paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES}
)

paraview_add_client_server_tests(
  TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
)

if (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
  paraview_add_client_server_render_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  paraview_add_client_server_render_tests(
    TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
  )
endif ()

# TODO: remote rendering tests and reverse connect tests.

paraview_add_multi_client_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES})

paraview_add_multi_server_tests(3
   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
   TEST_SCRIPTS ${TESTS_WITH_MULTI_SERVERS_3})

paraview_add_tile_display_tests(1 1
   TEST_SCRIPTS TileDisplaySplitView.xml)
if (PARAVIEW_USE_MPI)
  paraview_add_tile_display_tests(2 1
     TEST_SCRIPTS TileDisplaySplitView.xml)
  paraview_add_tile_display_tests(2 2
     TEST_SCRIPTS TileDisplaySplitView.xml)
  paraview_add_tile_display_tests(3 1
     TEST_SCRIPTS TileDisplaySplitView.xml)
endif()

paraview_add_cave_tests(2 "${CMAKE_CURRENT_SOURCE_DIR}/LeftRight.pvx"
  TEST_SCRIPTS TestCAVE.xml)

#------------------------------------------------------------------
# Add tests that test command line arguments (among other things).
#------------------------------------------------------------------
if (TARGET paraview)
  # The state file need to point to the correct data file. We do that by
  # configuring the state file.
  configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/exodusStateFile.3.14.1.pvsm.in"
    "${CMAKE_CURRENT_BINARY_DIR}/exodusStateFile.3.14.1.pvsm" @ONLY)
  paraview_add_client_tests(
    ARGS "--state=${CMAKE_CURRENT_BINARY_DIR}/exodusStateFile.3.14.1.pvsm"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS LoadExodusStateFile.xml)

  paraview_add_client_tests(
    ARGS "--state=${CMAKE_CURRENT_SOURCE_DIR}/boxClip.5.6.pvsm"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS BoxClipStateBackwardsCompatibility.xml)

  if (PARAVIEW_ENABLE_PYTHON)
    # Test whether or not we can load a Python state file with the --state
    # command line option as well as loading a Python state file that was
    # generated with PV 5.4.1. Loading the 5.4.1 Python state file checks
    # on backwards compatibility.
    set (Calculator54State_THRESHOLD 100)
    paraview_add_client_tests(
      ARGS "--state=${CMAKE_CURRENT_SOURCE_DIR}/Calculator54State.py"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS Calculator54State.xml)

    # Test backwards compatibility for Glyph filter change in ParaView 5.6
    paraview_add_client_tests(
      ARGS "--state=${CMAKE_CURRENT_SOURCE_DIR}/Glyph56State.py"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS Glyph56State.xml
    )
  endif()

  #----------------------------------------------------------------------
  # Add test to test stereo rendering modes.
  paraview_add_client_tests(
    CLIENT_ARGS --stereo --stereo-type=Interlaced
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StereoInterlaced.xml)
  paraview_add_client_server_tests(
    CLIENT_ARGS --stereo --stereo-type=Interlaced
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StereoInterlaced.xml)
  paraview_add_client_server_render_tests(
    CLIENT_ARGS --stereo --stereo-type=Interlaced
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StereoInterlaced.xml)
  paraview_add_client_tests(
    CLIENT_ARGS --stereo --stereo-type=SplitViewportHorizontal
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StereoSplitViewportHorizontal.xml)
  paraview_add_client_server_tests(
    CLIENT_ARGS --stereo --stereo-type=SplitViewportHorizontal
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StereoSplitViewportHorizontal.xml)
  paraview_add_client_server_render_tests(
    CLIENT_ARGS --stereo --stereo-type=SplitViewportHorizontal
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StereoSplitViewportHorizontal.xml)
endif()

#----------------------------------------------------------------------
# Tests that produce some output (other than rendered images) from ParaView
# that should be checked for correctness. These work by executing an XML
# test in the UI that produces some output, then pvpython runs a Python script
# that checks the file for correctness. Example use: checking a CSV
# file for expected content.
#
# Tests listed here are required to define the following files:
# - <test>.xml - XML test script to play in the uI
# - <test>Verify.py - Python code to verify output saved out in the XML
#   script
#----------------------------------------------------------------------
set(paraview_python_verify_tests
  ExportSceneSpreadSheetView
)

if (PARAVIEW_ENABLE_PYTHON)
  foreach(test_name ${paraview_python_verify_tests})
    set(tname "ParaView::Applications::${test_name}")
    add_test(NAME ${tname}
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
      -DDATA_DIR:PATH=${paraview_test_data_directory_output}
      -DTEST_NAME:STRING=${test_name}
      -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.xml
      -DTEST_VERIFIER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}Verify.py
      -DTEMPORARY_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -P ${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTestDriver.cmake
    )
    set_tests_properties(${tname} PROPERTIES LABELS "paraview")
  endforeach()
endif()
