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
  "DATA{${paraview_test_data_directory_input}/Data/AxisAlignedSliceBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00000.temp/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00000.temp/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00010.temp/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00010.temp/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00020.temp/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00020.temp/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00030.temp/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00030.temp/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00040.temp/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00040.temp/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00050.temp/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/double_mach_reflection/plt00050.temp/Level_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/paraview_icon.svg}"
  "DATA{${paraview_test_data_directory_input}/Data/3GQP.pdb}"
  "DATA{${paraview_test_data_directory_input}/Data/bake/bake.e}"
  "DATA{${paraview_test_data_directory_input}/Data/CameraWidgetViewLinkState.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/can-restarts/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/CellDataWithNan.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/cloud_layers_1k.jpg}"
  "DATA{${paraview_test_data_directory_input}/Data/CompositeGlyphInput.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/CompositeGlyphTree/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/CompositeGlyphTree.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/ConfigureCategories.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/convergecfd_post_5016_spray.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/cth.vtr}"
  "DATA{${paraview_test_data_directory_input}/Data/cube_1ts_mod.e}"
  "DATA{${paraview_test_data_directory_input}/Data/CubeStringArray.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/EnSight/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/ExRestarts/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FDS/test_core/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FieldDataBlocks/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FieldDataBlocks/FieldDataBlocks/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FileSeries/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/FileListPropertyWidgetPlugin.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/GenericIOReader/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/HeadMRVolume.mhd}"
  "DATA{${paraview_test_data_directory_input}/Data/HeadMRVolume.raw}"
  "DATA{${paraview_test_data_directory_input}/Data/hdf_fpm_simulation.erfh5}"
  "DATA{${paraview_test_data_directory_input}/Data/half_sphere_commented.csv}"
  "DATA{${paraview_test_data_directory_input}/Data/Iron_Xdmf/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/LoadState.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/LoadStateDialog.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/LoadStateTextSourceRepresentation.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/MinimalTensors.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiBlockWithPieces.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiBlockWithPieces_0_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiBlockWithPieces_1_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/NE2_ps_bath.png}"
  "DATA{${paraview_test_data_directory_input}/Data/OMETIFF/multi-channel-time-series.ome.tif}"
  "DATA{${paraview_test_data_directory_input}/Data/OverridePropertyPlugin.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/PlotOverLineLegacy.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/PLOAD_C_xMax.bdf}"
  "DATA{${paraview_test_data_directory_input}/Data/PNGReadersPlugin.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/PropertyLink.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/ReaderNamePlugin.xml}"
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
  "DATA{${paraview_test_data_directory_input}/Data/VisItBridge/chombo_hdf5_series/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/nek5000/eddy_uv/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/amr/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/amr/wavelet/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/artifact.dta}"
  "DATA{${paraview_test_data_directory_input}/Data/amr_gaussian_pulse.vtkhdf}"
  "DATA{${paraview_test_data_directory_input}/Data/blow.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/blow_data.myvtk}"
  "DATA{${paraview_test_data_directory_input}/Data/bluntfin.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/BoxWithFaceData.cgns}"
  "DATA{${paraview_test_data_directory_input}/Data/can.ex2}"
  "DATA{${paraview_test_data_directory_input}/Data/can.e.4/can.e.4.0}"
  "DATA{${paraview_test_data_directory_input}/Data/can.e.4/can.e.4.1}"
  "DATA{${paraview_test_data_directory_input}/Data/can.e.4/can.e.4.2}"
  "DATA{${paraview_test_data_directory_input}/Data/can.e.4/can.e.4.3}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/system/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/constant/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/0/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/0.5/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/1/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/1.5/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/2/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cavity/2.5/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cgns_np01/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/cgns_np04/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/bc_struct.cgns}"
  "DATA{${paraview_test_data_directory_input}/Data/comb.q}"
  "DATA{${paraview_test_data_directory_input}/Data/combxyz.bin}"
  "DATA{${paraview_test_data_directory_input}/Data/cow.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/cth.vtr}"
  "DATA{${paraview_test_data_directory_input}/Data/cube.off}"
  "DATA{${paraview_test_data_directory_input}/Data/cube.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/cylinder_run1.e}"
  "DATA{${paraview_test_data_directory_input}/Data/cylinder_run2.e}"
  "DATA{${paraview_test_data_directory_input}/Data/disk_out_ref.ex2}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation.clone.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/dualSphereAnimation4.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/elements.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/EngineSector.cgns}"
  "DATA{${paraview_test_data_directory_input}/Data/ensemble-wavelet/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Fides/cartesian-attr.bp}"
  "DATA{${paraview_test_data_directory_input}/Data/Fides/cartesian-attr.bp.dir/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Fides/tris-blocks-time-attr.bp}"
  "DATA{${paraview_test_data_directory_input}/Data/Fides/tris-blocks-time-attr.bp.dir/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/field.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/filters.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/AnimatedMorphCube/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/BoxAnimated/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/InterpolationTest/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/NestedRings/NestedRings.glb}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/Triangle/Triangle.glb}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/SimpleSkin/SimpleSkin.gltf}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/WaterBottle/WaterBottle.glb}"
  "DATA{${paraview_test_data_directory_input}/Data/HTG/binary_3D_333_mask.htg}"
  "DATA{${paraview_test_data_directory_input}/Data/HTG/donut_XZ_shift_2d.htg}"
  "DATA{${paraview_test_data_directory_input}/Data/HTG/ghost.htg}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGCellCentersBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGFeatureEdgesBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGGhostCellsGeneratorBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGReflectionBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGVisibleLeavesSizeBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGMulti.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGMulti/HTGMulti_0_0.htg}"
  "DATA{${paraview_test_data_directory_input}/Data/HTGMulti/HTGMulti_1_0.htg}"
  "DATA{${paraview_test_data_directory_input}/Data/ReflectBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/WaveletGradientLegacy.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/img1.png}"
  "DATA{${paraview_test_data_directory_input}/Data/img1.mypng}"
  "DATA{${paraview_test_data_directory_input}/Data/img2.png}"
  "DATA{${paraview_test_data_directory_input}/Data/iron_protein.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/linesPolydata.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/LogoSource.png}"
  "DATA{${paraview_test_data_directory_input}/Data/manyTypes.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces/pvi_Material_Remap2_0005_0_0.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces/pvi_Material_Remap2_0005_0_1.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces/pvi_Material_Remap2_0005_1_0.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces/pvi_Material_Remap2_0005_1_1.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces/pvi_Material_Remap2_0005_2_0.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mb_with_pieces/pvi_Material_Remap2_0005_2_1.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/mg_diff_0062.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/mg_diff_0062/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_0.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_1.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_2.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece/Piece1.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece/Piece2.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece/Piece3.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece/Piece4.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece/Piece5.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/MultiPiece/Piece6.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/non_convex_polygon.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/office.binary.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/owave-hole.exdg}"
  "DATA{${paraview_test_data_directory_input}/Data/OMF/test_file.omf}"
  "DATA{${paraview_test_data_directory_input}/Data/OSPRayMaterials/materials.json}"
  "DATA{${paraview_test_data_directory_input}/Data/pbrSpheres.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/pbrSpheresAlbedo.jpg}"
  "DATA{${paraview_test_data_directory_input}/Data/pbrSpheresORM.jpg}"
  "DATA{${paraview_test_data_directory_input}/Data/pointSet.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/ParaView_Mark.png}"
  "DATA{${paraview_test_data_directory_input}/Data/ParaView_Logo.png}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_0_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_1_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_2_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_3_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_4_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_5_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_6_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_7_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_8_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_9_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_10_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_11_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_12_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_13_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData/PartialFieldData_14_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/PartialFieldDataMultiBlock/PartialFieldData.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/PolarAxesDeprecation.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/porphyrin.cml}"
  "DATA{${paraview_test_data_directory_input}/Data/ProcessIdScalarsBackwardCompat.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/quadraticTetra01.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/quadratic_tets_with_ghost_cells.pvtu}"
  "DATA{${paraview_test_data_directory_input}/Data/quadratic_tets_with_ghost_cells_0.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/quadratic_tets_with_ghost_cells_1.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/random_torus.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/room.cas}"
  "DATA{${paraview_test_data_directory_input}/Data/room.dat}"
  "DATA{${paraview_test_data_directory_input}/Data/room.cas.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/room.dat.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/sample.h5part}"
  "DATA{${paraview_test_data_directory_input}/Data/servers-wiki.pvsc}"
  "DATA{${paraview_test_data_directory_input}/Data/Udirectory/coneScript.py}"
  "DATA{${paraview_test_data_directory_input}/Data/Udirectory/simpleScript.py}"
  "DATA{${paraview_test_data_directory_input}/Data/Udirectory/LinkRenderViewsCheckSize.py}"
  "DATA{${paraview_test_data_directory_input}/Data/Udirectory/resetSession.py}"
  "DATA{${paraview_test_data_directory_input}/Data/rectilinear_grid_of_pixel.vtr}"
  "DATA{${paraview_test_data_directory_input}/Data/sineWaves.csv}"
  "DATA{${paraview_test_data_directory_input}/Data/singleSphereAnimation.pvd}"
  "DATA{${paraview_test_data_directory_input}/Data/singleSphereAnimation/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/spiaggia_di_mondello_1k.hdr}"
  "DATA{${paraview_test_data_directory_input}/Data/SliceDeprecation.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SteeringDataGeneratorPlugin.xml}"
  "DATA{${paraview_test_data_directory_input}/Data/squareBend/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/squareBend/system/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/squareBend/constant/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/squareBend/50/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/squareBend/100/,RECURSE:,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/sphere.stl}"
  "DATA{${paraview_test_data_directory_input}/Data/tensors.pvti}"
  "DATA{${paraview_test_data_directory_input}/Data/test_fielddata.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/test_transient_poly_data.vtkhdf}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartdscol/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartdscol.vtpc}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartds/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/testxmlpartds.vtpd}"
  "DATA{${paraview_test_data_directory_input}/Data/time_source.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/timestep_0_15.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/todd2_small/todd2_htg.pio}"
  "DATA{${paraview_test_data_directory_input}/Data/todd2_small/todd2_ug.pio}"
  "DATA{${paraview_test_data_directory_input}/Data/todd2_small/dumps/3D_nomix_diff-dmp000000}"
  "DATA{${paraview_test_data_directory_input}/Data/todd2_small/dumps/3D_nomix_diff-dmp000010}"
  "DATA{${paraview_test_data_directory_input}/Data/tube.exo}"
  "DATA{${paraview_test_data_directory_input}/Data/Unbalanced3DepthOctTree3x2x3.htg}"
  "DATA{${paraview_test_data_directory_input}/Data/vehicle_data.csv}"
  "DATA{${paraview_test_data_directory_input}/Data/viscoplastic-ring.h5}"
  "DATA{${paraview_test_data_directory_input}/Data/vtkHDF/pdc_multi.vtkhdf}"
  "DATA{${paraview_test_data_directory_input}/Data/waveletElevation.vti}"
  "DATA{${paraview_test_data_directory_input}/Data/waveletMaterial.vti}"
  "DATA{${paraview_test_data_directory_input}/Data/WaveletThresholdLegacy.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/x_ray_copy_1.json}"
  "DATA{${paraview_test_data_directory_input}/Data/x_ray_copy_2.json}"
  "DATA{${paraview_test_data_directory_input}/Data/YoungsMaterialInterface/youngs.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/YoungsMaterialInterface/youngs/youngs_0.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/YoungsMaterialInterface/youngs/youngs_1.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/ugrid.nc}"
  "DATA{${paraview_test_data_directory_input}/Data/UnstructuredGridWithTwoAdjacentHexahedraOfDifferentOrder.vtu}"
  "DATA{${paraview_test_data_directory_input}/Data/DecimatePolyline.vtp}"

  # Baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AAXAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AAYAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AAZAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AnisotropyPBR.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid7.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid8.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid9.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxesGrid10.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterPDCNoHierarchy.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterPDCNoHierarchy_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterPDCNoHierarchyYAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterPDCNoHierarchyYAxis_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterMBHierarchy.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterMBHierarchy_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterMBHierarchyYAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedCutterMBHierarchyYAxis_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/AxisAlignedReflect.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BatchAxesGrid.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-bake-HideOneBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-bake-HideTwoBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-bake-ShowAllBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-bake-ShowOneBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-bake-ShowZeroBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-BlockOpacity.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-ShowAllBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-ShowOnlyBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockContextMenu-UnsetBlockOpacity.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockLinkedSelection-MB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockLinkedSelection-MBWithPieces.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockLinkedSelection-PDC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/BlockLinkedSelection-PDC_1.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CategoriesWithNaN.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CGNSReaderCellMesh.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CGNSReaderFaceMesh.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsE.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsF.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsG.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartAxisRangeAndLabelsH.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartDefaultSettings_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartDefaultSettings_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesA_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartLoadNoVariablesB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ChartsClearSelection.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ClampAndUpdateColorMap-TS12-Clamped.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ClampAndUpdateColorMap-TS12-Unchanged.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ClampAndUpdateColorMap-TS6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorBy.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityEditorFreehandDrawing.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityEditorFreehandDrawing_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityEditorRangeHandles.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityEditorRangeHandles_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityEditorRangeHandles_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_D.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_E.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_F.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_G.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_M.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_R.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_T.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_U.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorOpacityTableEditorHistogram_V.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorLegendBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ColorLegendScaledScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ComputeNormalInRepresentation-Default.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ComputeNormalInRepresentation-SurfaceEdgesRepr.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ComputeNormalInRepresentation-WithAngle.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ConnectivityCountAscending.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ConnectivityCountDescending.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour3_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Contour3_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ContourRange_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ContourRange_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ContourCellData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CONVERGECFDCGNSReader.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CONVERGECFDReaderMesh.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CONVERGECFDReaderMesh_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CONVERGECFDReaderParcels.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CONVERGECFDReaderParcels_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ConvertToMoleculeBonds.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CriticalTimeBetweenX.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CriticalTimeBetweenZ.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CriticalTimeLowerMagnitude.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CriticalTimeUpperMagnitude.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomTCoords.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomViewpoints.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomViewpoints_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/CustomViewpoints_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DataAxesGrid2X.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DataAxesGrid-Custom.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DataAxesGrid-CustomWithReset.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DecimatePolylineAngle.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DecimatePolylineCustomFieldHigh.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DecimatePolylineCustomFieldLow.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DecimatePolylineDistance.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DeleteSubtreePipelineBrowser0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DeleteSubtreePipelineBrowser1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/DeleteSubtreePipelineBrowser2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EdgeTintPBR.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EDLWithSubsampling.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EDLWithSubsampling_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EllipseSource1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EllipseSource2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EllipseSource3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnsembleT0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnsembleT1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnsembleT3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EqualizeLayoutA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EqualizeLayoutB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExpandSelection_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExpandSelection_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractCellsByType.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractCellsByType2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractCellsByType3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractSubsetWithSeed.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractSubsetWithSeed_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractSubsetWithSeed_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractSubsetWithSeed_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExodusModeShapes_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExodusModeShapes_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EyeDomeLighting.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FastUniformGridSource4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FeatureEdgesFilterHTG.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FeatureEdgesHTG.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FeatureEdgesHTGNoMask.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FeatureEdgesNonlinearCells.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FeatureEdgesRepresentationHTG.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FidesReader_cartesian.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FidesReader_blocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FidesWriter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FileSeries1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FileSeries2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Cell-GlobalElementId-is.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Cell-GlobalElementId-oneof.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Cell-GlobalElementId-range.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Cell-ID-is.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Cell-ID-min-max-per-block.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-ge.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-ge-mean.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-le.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-le-mean.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-max.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-mean.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-Temp-min.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataQueries-Point-MultipleQueries.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataSelectLocation.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataSelectLocationMultiblock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FindDataDialogNaN.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FreezeQueryMultiblock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ForceTimeDiamond.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFImporterNestedRings.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFImporterWaterBottle.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderTriangle.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderAnimatedMorphing.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderAnimatedMorphing_IsLoaded.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderMultipleScenes_IsLoaded.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderMultipleScenes_SceneOne.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderMultipleScenes.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderMultipleAnimations_IsLoaded.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderMultipleAnimations_ThreeAnimations.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderMultipleAnimations.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderToggleDeformation_IsLoaded.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderToggleDeformation_IsSkinned.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/glTFReaderToggleDeformation.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostCellsGeneratorImageDistributed.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostCellsGeneratorImageSerial.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe7.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe8.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe9.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/GhostFeatureEdgesAndWireframe10.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGrid2DGeometryFillMaterial_Filled.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGrid2DGeometryFillMaterial_Lines.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisClip1-Plane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisClip2-RevPlane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisClip3-Box.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisCut.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridAxisReflection.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridCellCenters.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridContour.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridContour_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridContourStrategyA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridContourStrategyA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridContourStrategyB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridContourStrategyB_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridDataAndPolarAxes_Data.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridDataAndPolarAxes_Polar.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridDepthLimiter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridEvaluateCoarse.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridEvaluateCoarse_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridExtractGhostCells.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridGeometry.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridGradient.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridMultiBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridMultipleClip.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridObliquePlaneCutter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridPlaneCutter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSourceDistributedA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSourceDistributedB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionA_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionB_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionB_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionD_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridSurfaceMultiBlockSelectionD_2.png}"
  #"DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridThreshold.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridThreshold-DepthOne.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/HyperTreeGridThreshold-DepthTwo.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/IndexedLookupInitialization.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/IndexedLookupInitialization_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LightAddRemove1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LightAddRemove2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LightWidget.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LightToolbar.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LineChartSelection4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LinkCameraFromView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LinkCameraFromView_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LinkCameraFromView_A_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LinkCameraFromView_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LinkViewsLinked.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogoSource.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogoSourceDefault.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LogoSourcesInChartViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MatchBoundariesIgnoringCellOrder.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorCreateMaterials1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorCreateMaterials2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorLoadMaterials1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorLoadMaterials2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorSaveMaterials1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorSaveMaterials2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorSaveMaterials3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorShaderBallScene1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorShaderBallScene2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MaterialEditorShaderBallScene3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MathTextColumn.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MergePointBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MergeVectorComponents.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Molecule-BallStick.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Molecule-Custom.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Molecule-Liquorice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockChartSelectionPlotView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockChartSelectionRenderView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorMultiBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_D.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_E.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_F.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_G.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_H.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_I.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_J.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorProperties_K.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorSelection-SelectBlock2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorSelection-SelectElementBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorSelection-HideBlock1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiBlockInspectorWithoutSelectOnClickSetting.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultipleColorOnSelectionA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NestedViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NestedViews_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/NetCDFUGRID.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EmptyInitialCompositeReader.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OBBMajorAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OBBMediumAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OBBMinorAxis.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OffsetSliceWithPlane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OMFReader.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/OpacityWidgetRange.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ParallelCoordinatesView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ParallelCoordinatesView_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSet_Slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSet_Slice_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSet_Slice_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSet_Surface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Slice_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Slice_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Slice_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PartitionedDataSetCollection_Surface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PBRSpheres.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PickCenter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PickCenter_Vertex.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PLYWriter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteBlackBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteBlueGrayBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteGradientBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteLightGrayBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteNeutralGrayBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteWarmGrayBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteDarkGrayBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PaletteWhiteBackground.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PlotOverLine_Boundary.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PlotOverLine_Center.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PlotOverLine_Uniform.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PointGaussianScaleOpacityArrayA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PointGaussianScaleOpacityArrayA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PolarAxes1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PolarAxes2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PolarAxes3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Preview-SingleView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Preview-AllViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PythonAlgorithmPlugin.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PythonAlgorithmReadersAndWriters.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PythonShellRunScript.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/PVCellCentersHyperTreeGrid.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/QuartilePlotArea.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/QuartilePlotArea_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/QuartilePlotLines.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RandomAttributesHTGScalars.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RandomAttributesHTGVectors.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RandomHyperTreeGridSourceA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RandomHyperTreeGridSourceA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RandomHyperTreeGridSourceB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_BCOnly.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_BCOnlyBlockColors.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_CellData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_LoadPatches.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadPartitionedCGNS_PointData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadCGNSBCDataset.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadIOHDFFileSeries0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadIOHDFFileSeries1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadIOHDFFileSeries2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadIOHDFWithCache0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadIOHDFWithCache1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadIOHDFWithCache2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReadFLUENTCFFFormat.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RectilinearVolumeRendering.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReloadAMReXGrid-AfterReload.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReloadAMReXGrid-BeforeReload.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReloadExodusFile-AfterReload.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ReloadExodusFile-BeforeReload.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingSurface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingSurface_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingSurface_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RemoteRenderingVolume.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderNanDefaultColor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderNanGUIColor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderNanPresetColor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderViewContextMenuArrayColoring.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RenderViewContextMenuSolidColoring.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleToHyperTreeGridBranching_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleImageToHyperTreeGrid.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleToHyperTreeGrid2DX.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleToHyperTreeGrid2DY.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleToHyperTreeGrid2DZ.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ResampleToLine.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RotateAroundOriginTransform.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RulerD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveLargeScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveLargeScreenshot_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveSmallScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveSmallScreenshot_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar6.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar7.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ScalarBar8.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectCellsFrustumTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectCellsInteractiveTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectCellsTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectColorMapFromComboBox_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectionByArray.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectionEditor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectionLinkParallelCoordinatesView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectionLinkScriptingA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectionLinkScriptingB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectPointsFrustumTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectPointsInteractiveTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SelectPointsTrace.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparatedColorMapA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparatedColorMapB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparateOpacityArray_X.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparateOpacityArray_Y.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparateOpacityArray_Z.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeparateOpacityArray_Magnitude.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeriesPreset.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeriesPresetRegexpNoRegexp.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeriesPresetRegexpYD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SeriesPresetRegexp.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ShowMultiPieceFieldData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceDirectDoubleColoring.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceRepresentation.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlaneCellData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlaneMultiBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SliceWithPlane.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SpherePointSource_InterA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SpherePointSource_InterB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SplitViewTraceScreenshot.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SSAO.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SteeringDataGeneratorA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SteeringDataGeneratorB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SteeringDataGeneratorC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/StepColorSpaceA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/StructuredGridCellBlanking.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/StructuredGridVolumeRendering.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TableFFT1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TableFFT2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TemporalSmoothingHalfWidthSmall.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TemporalSmoothingHalfWidthLarge.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestAnnotateSelectionFilter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCAVE-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCAVE-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCAVEVolRen-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCAVEVolRen-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCleanGridPointDataStrategies_0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCleanGridPointDataStrategies_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCleanGridPointDataStrategies_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestColorMapModificationRendersAllViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestClipCylinder.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestColorHistogram.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCompositedGeometryCulling.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestGeometryBoundsClobber.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestERFReader.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t0_boundaries_cc.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t0_boundaries.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t0_slice_cc.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t0_slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t8_boundaries_cc.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t8_boundaries.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t8_slice_cc.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestFDSReader_t8_slice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGSelection_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGSelection_A_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGSelection_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGSelection_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGSelection_D.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGThreshold.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGThreshold_1.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestOFFReader.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPreviewTextScale.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPreviewTextScale_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonView.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonView_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonViewScript.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestResampleHyperTreeGridWithSphere.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TextSourceBorder.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TextSourceInteriorLines.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TextSourcesInChartViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TextSourcesInChartViews2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TextSourcesInChartViews3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Threshold.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdAllComponents.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdAnyComponent.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdInvert.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdSelectedComponentX.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdSelectedComponentY.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdSelectedComponentZ.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ThresholdSelectedMagnitude.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-1x1-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x1-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x1-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-2x2-tile3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile0.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TileDisplaySplitView-3x1-tile2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TimeStepProgressBar_Inter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ToneMapping.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceExodus.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceMultiViews.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceMultiViews_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TraceTimeControls.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunctionResetOnVisibilityChange_AllBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunctionResetOnVisibilityChange_OneBlock.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunctionResetOnVisibilityChange_TwoBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DEditor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DEditor_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DEditor_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalars.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalars_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalars_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalars_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditor.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditor_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorA_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorB_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorB_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TransferFunction2DYScalarsEditorB_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TriangleStrips.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UniformInverseTransformSamplingGlyph_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UniformInverseTransformSamplingGlyph_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UniformInverseTransformSamplingGlyph_C.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UnstructuredVolumeVectorComponent_Magnitude.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UnstructuredVolumeVectorComponent_X.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UnstructuredVolumeVectorComponent_Y.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/UnstructuredVolumeVectorComponent_Z.png}"
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
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeCrop.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeOfRevolution.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeIsosurfaceBlendMode.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeNoMapScalars.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VolumeSliceBlendMode.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/VTKHDFWriter.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/WaveNoFontScale.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/WaveFontScale.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/YoungsMaterialInterface.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_ChangeParameters.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_Default.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_DisableBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYChart_EnableBlocks.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_Default.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_UseIndexForX.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_UseIndexForX_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResult.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResult2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResultChanged.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/XYHistogram_WithResultChangedNormalized.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomClosestA.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomClosestA_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomClosestB.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomClosestC.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomClosestD.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomToData.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomToData_A.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomToData_B.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ZoomToEmptyData.png}"
)

# Set INTEL_MACOS so we can avoid running some tests on macos
# with intel CPUs. This is because there are OpenGL issues and
# intel-based macs are end-of-life soon.
set(INTEL_MACOS FALSE)
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  # This is the most reliable detection method but is specific to Kitware's CI machines.
  set(INTEL_MACOS TRUE)
elseif(APPLE AND "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
  # Builders may report arm architectures even when targeting x86_64 machines
  # depending on the cmake executable's architecture. However, this is likely to
  # work outside of Kitware's CI infrastructure.
  set(INTEL_MACOS TRUE)
endif()

# Some tests can take a long time; increase their timeouts.
# Really very long (300 seconds):
set(SeparateOpacityArray_TIMEOUT 300)
set(TransferFunction2DYScalars_TIMEOUT 300)
set(UndoRedo1_TIMEOUT 300)
set(GhostFeatureEdgesAndWireframe_TIMEOUT 300)

# Very long (200 seconds):
set(EqualizeLayout_TIMEOUT 200)

# Somewhat long (150 seconds):
set(AMReXParticlesReader_TIMEOUT 150)
set(Clip_TIMEOUT 150)
set(ColorOpacityEditorFreehandDrawing_TIMEOUT 150)
set(ColorOpacityTableEditorHistogram_TIMEOUT 150)
set(ComparativeVisPanel_TIMEOUT 150)
set(Ensemble_TIMEOUT 150)
set(HelpWindowHistory_TIMEOUT 150)
set(ImageVolumeRendering_TIMEOUT 150)
set(LagrangianIntegrationModel_TIMEOUT 150)
set(LagrangianParticleTrackerReseeding_TIMEOUT 150)
set(LagrangianParticleTracker_TIMEOUT 150)
set(MemoryInspectorPanel_TIMEOUT 150)
set(MultiBlockVolumeRendering_TIMEOUT 150)
set(MultiServer3_TIMEOUT 150)
set(PropertyContextMenu_TIMEOUT 150)
set(ProxyCategoriesConfiguration_TIMEOUT 150)
set(ProxyCategoriesDialog_TIMEOUT 150)
set(SaveTSV_TIMEOUT 150)
set(ScalarBarRange_TIMEOUT 150)
set(SeparatedColorMapOpacity_TIMEOUT 150)
set(SliceRepresentation_TIMEOUT 150)
set(SplitViewTrace_TIMEOUT 150)
set(SpreadSheet3_TIMEOUT 150)
set(TemporalParticleKeepDeadTrails_TIMEOUT 150)
set(TraceExportAndSaveData_TIMEOUT 150)
set(TraceMultiViews_TIMEOUT 150)
set(TransferFunction2D_TIMEOUT 150)
set(ZoomToData_TIMEOUT 150)

list (APPEND TESTS_WITHOUT_BASELINES
  AboutDialog.xml
  AddFieldArrays.xml
  AnimateProperty.xml
  ArrayBoundsScaleDomain.xml
  ArraySelectionWidget.xml
  AutoConvertSingleProperty.xml
  ComputeArrayMagnitudeSetting.xml
  CSVPreview.xml
  CellQualityForPixel.xml
  CustomOutputFormat.xml
  DefaultReadersSetting.xml
  DelimitedTextReader.xml
  DynamicFieldDataDomain.xml
  ExplodeDataSet.xml
  ExportFilteredColumnsSpreadsheet.xml
  ExportSpreadsheetFormatting.xml
  ExpressionsDialog.xml
  ExpressionsExporter.xml
  ExpressionsSettings.xml
  ExtractParticlesOverTime.xml
  FieldDataToAttribute.xml
  FileDialogOpenInteractClose.xml
  FluentReaderZoneSelection.xml
  ForceStaticMesh.xml
  GradientBackwardsCompatibility.xml
  HTGCellCentersBackwardCompat.xml
  HTGFeatureEdgesBackwardCompat.xml
  HTGGhostCellsGeneratorBackwardCompat.xml
  HTGReflectionBackwardCompat.xml
  HTGVisibleLeavesSizeBackwardCompat.xml
  ImportCustomPresets.xml
  InformationPanel.xml
  IntegrationStrategy.xml
  LogViewer.xml
  MergeTime.xml
  ModifyGUIWindowTitle.xml
  OpacityWidgetRange.xml
  OpenHelp.xml
  OverrideProperty.xml
  PartialArrayInLineChart.xml
  PlotOverLineLegacy.xml
  PointCellDataConversion.xml
  PolarAxesBackwardsCompatibility.xml
  PreservePropertyValues.xml
  ProcessIdScalarsBackwardCompat.xml
  ProxyCategoriesConfiguration.xml
  ProxyCategoriesDialog.xml
  ProxyCategoriesDialogDefault.xml
  ProxyCategoriesDialogSettings.xml
  ProxyCategoriesFavorites.xml
  ProxyCategoriesMenusDefault.xml
  ProxyCategoriesToolbar.xml
  QuadraturePointsDS.xml
  QuadricDecimation.xml
  QuickLaunchCreateProxy.xml
  QuickLaunchNavigation.xml
  QuickLaunchRequest.xml
  ReadIOHDFAMRMaxLevels.xml
  ReaderRegistrationName.xml
  ReflectBackwardCompat.xml
  ReloadCGNSFile.xml
  RescaleVisibleRangeOption.xml
  SearchBox.xml
  ServerConnectDialog.xml
  SettingsNoCustomDefault.xml
  SettingsOverrideDomain.xml
  SettingsProxyProperty.xml
  SettingsRestoreProxyAppDefault.xml
  SettingsSubProxy.xml
  SliceBackwardsCompatibility.xml
  SpatioTemporalHarmonicsAttribute.xml
  SpatioTemporalHarmonicsSource.xml
  SpreadSheetFieldData.xml
  SpreadSheetSelectedCellConnectivity.xml
  STLReaderMergePoints.xml
  StringInLineChartView.xml
  TableHistogram.xml
  TemporalArrayOperator.xml
  TensorPrincipalInvariants.xml
  TestCompositePropertyWidgetDecorator.xml
  TestFileListPropertyWidget.xml
  TestOpenFOAMRestartFiles.xml
  TextSource.xml
  ThresholdBackwardsCompatibility.xml
  ThresholdTable.xml
  TimeKeeper.xml
  TimeManagerPanel.xml
  TimeManagerSequence.xml
  WriteXMLTimeValue.xml
  YieldCriteria.xml
)

if (PARAVIEW_PLUGIN_ENABLE_EULATestPlugin)
  list(APPEND TESTS_WITHOUT_BASELINES
    PluginEULA.xml)
endif ()

if (PARAVIEW_ENABLE_OPENVDB)
  list (APPEND TESTS_WITHOUT_BASELINES
    OpenVDBReader.xml
  )
endif ()

if (PARAVIEW_ENABLE_CATALYST)
  list (APPEND TESTS_WITHOUT_BASELINES
    CatalystLiveConnection.xml
  )
endif ()

if (PARAVIEW_USE_QTHELP)
  list(APPEND TESTS_WITHOUT_BASELINES
    HelpWindowHistory.xml)
endif()

if (WIN32)
  list(APPEND TESTS_WITHOUT_BASELINES
    AVIWriter.xml
    MP4Writer.xml)
endif()

list (APPEND TESTS_WITH_INLINE_COMPARES
  AxesGrid.xml
  AxisAlignedCutterMBHierarchy.xml
  AxisAlignedCutterPDCNoHierarchy.xml
  AxisAlignedReflect.xml
  BoundingRuler.xml
  BoxWidget.xml
  BlockContextMenu.xml
  BlockLinkedSelection.xml
  CalculatorQuotedVariable.xml
  CameraLink.xml
  CGNSReaderDataLocation.xml
  CGNSReaderSurfacePatches.xml
  ChartDefaultSettings.xml
  ChartLoadNoVariables.xml
  ClampAndUpdateColorMap.xml
  ColorOpacityTableEditorHistogram.xml
  ComputeNormalsInRepresentation.xml
  Connectivity.xml
  Contour.xml
  ContourRange.xml
  CriticalTime.xml
  DataAxesGrid2X.xml
  DecimatePolyline.xml
  DeleteSubtreePipelineBrowser.xml
  EllipseSource.xml
  Ensemble.xml
  EqualizeLayout.xml
  ExtractSubsetWithSeed.xml
  FastUniformGridSource.xml
  FeatureEdgesHTG.xml
  FeatureEdgesHTGNoMask.xml
  FileSeries.xml
  FontScalingInScreenshots.xml
  HyperTreeGrid2DGeometryFillMaterial.xml
  HyperTreeGridAxisClip.xml
  HyperTreeGridAxisCut.xml
  HyperTreeGridAxisReflection.xml
  HyperTreeGridCellCenters.xml
  HyperTreeGridContour.xml
  HyperTreeGridContourStrategy.xml
  HyperTreeGridDataAndPolarAxes.xml
  HyperTreeGridDepthLimiter.xml
  HyperTreeGridEvaluateCoarse.xml
  HyperTreeGridExtractGhostCells.xml
  HyperTreeGridGeometry.xml
  HyperTreeGridGradient.xml
  HyperTreeGridMultiBlock.xml
  HyperTreeGridMultipleClip.xml
  HyperTreeGridObliquePlaneCutter.xml
  HyperTreeGridPlaneCutter.xml
  HyperTreeGridThreshold.xml
  HyperTreeGridSurfaceMultiBlockSelection.xml
  LightAddRemove.xml
  LightToolbar.xml
  LineChartSelection.xml
  LoadStateMultiView.xml
  LogColorMap.xml
  LogColorMapToggle.xml
  LogoSourcesInChartViews.xml
  MemoryInspectorPanel.xml
  Molecule.xml
  MultiBlockInspectorMultiBlock.xml
  MultiBlockInspectorProperties.xml
  MultiBlockInspectorSelection.xml
  MultiBlockChartSelection.xml
  MultipleColorOnSelection.xml
  NestedViews.xml
  NonlinearSubdivisionDisplay.xml
  PaletteBackgrounds.xml
  ParallelCoordinatesView.xml
  PartitionedDataSet.xml
  PartitionedDataSetCollection.xml
  PlotOverLine_3d.xml
  Preview.xml
  PreviewFontScaling.xml
  PropertyContextMenu.xml
  QuartilePlot.xml
  RandomHyperTreeGridSource.xml
  ReadPartitionedCGNS.xml
  ReadCGNSBCDataset.xml
  ReadIOHDFFileSeries.xml
  ReadIOHDFWithCache.xml
  ReadFLUENTCFFFormat.xml
  RectilinearFractal.xml
  ReloadAMReXGrid.xml
  ReloadExodusFile.xml
  RemoteRendering.xml
  RenameArrays.xml
  RenderNan.xml
  RenderViewContextMenu.xml
  ResetToVisibleRange.xml
  Ruler.xml
  SaveExodus.xml
  SaveLargeScreenshot.xml
  ScalarBarRange.xml
  SelectColorMapFromComboBox.xml
  SelectionEditor.xml
  SelectionLinkParallelCoordinatesView.xml
  SeparateOpacityArray.xml
  SliceRepresentation.xml
  SteeringDataGenerator.xml
  TemporalSmoothing.xml
  TestCleanGridPointDataStrategies.xml
  TestColorMapModificationRendersAllViews.xml
  TestERFReader.xml
  TestHTGSelection.xml
  TestHTGThreshold.xml
  TestIsoVolume.xml
  TextSourceBorder.xml
  Threshold.xml
  ThresholdComponentModes.xml
  TransferFunctionResetOnVisibilityChange.xml
  UnstructuredVolumeRenderingVectorComponent.xml
  UpdateTransferFunctionRanges.xml
  XYChart.xml
  XYHistogram.xml
  ZoomClosest.xml
)

# Image data delivery to render server not supported.
set(SeparateOpacityArray_DISABLE_CRS TRUE)

if(NOT APPLE)
  # Workaround to avoid testing failures on high-res Retina displays on Macs
  list(APPEND TESTS_WITH_INLINE_COMPARES
    ColorLegendScaledScreenshot.xml
  )
endif()

list(APPEND TESTS_WITH_BASELINES
  AnnulusWidget.xml
  AxisAlignedCutterAMR.xml
  AxisAlignedCutterHTG.xml
  BDFReader.xml
  BoxWidgetVisibleBlock.xml
  CameraOrientationWidget.xml
  ChartsClearSelection.xml
  CleanToGridRemovePoints.xml
  ConeWidget.xml
  ConstantShadingPerObject.xml
  CONVERGECFDCGNSReader.xml
  DataAxesGrid.xml
  EmptyInitialCompositeReader.xml # Issue #21293
  FeatureEdgesFilterHTG.xml
  FeatureEdgesRepresentationHTG.xml
  FrustumWidget.xml
  IOSSMergeExodusEntityBlocks.xml
  ImageChartView.xml
  IntegrateVariablesPDC.xml
  LogoSource.xml
  MergeVectorComponents.xml
  MissingPartialArraysColoredByNaN.xml
  MoleculeScalarBar.xml
  MultiSliceHTG.xml
  NetCDFUGRID.xml
  PartialFieldDataMultiBlock.xml
  ParticleTracerGlyph.xml
  PassArrays.xml
  PerlinNoise.xml
  PointAndCellIdsHTG.xml
  Protractor.xml
  PVCellCentersHyperTreeGrid.xml
  RegionIds.xml
  RemoveGhostInformationHTG.xml
  ResampleToLine.xml
  ReverseSense.xml
  SaveLoadScreenShotWithEmbedState.xml
  SelectionBlocksThrough.xml
  SelectionByArray.xml
  SeriesPreset.xml
  SeriesPresetRegexp.xml
  ShowMultiPieceFieldData.xml
  TemporalParticleKeepDeadTrails.xml
  TestHiddenLineRemoval.xml
  TestOpenFOAMWeighByCell.xml
  TestResampleHyperTreeGridWithSphere.xml
  TransferFunction2D.xml
  UnlinkCameraView.xml
  VolumeOfRevolution.xml
  ZoomClosestOffsetRatio.xml
  ZoomToBox.xml
  ZoomToData.xml
  ZoomToEmptyData.xml
  PCANormalEstimation.xml
)

if (NOT INTEL_MACOS)
  list(APPEND TESTS_WITH_INLINE_COMPARES
    PickCenter.xml
  )
  list(APPEND TESTS_WITH_BASELINES
    IOSSCellGridHCurl.xml
    IOSSCellGridHDiv.xml
    TransferFunction2DYScalars.xml
  )
endif()

if(PARAVIEW_ENABLE_VISITBRIDGE)
  list(APPEND TESTS_WITH_BASELINES
    VisItBridgeChombo.xml
    VisItBridgeChomboFileSeries.xml
    VisItBridgeLAMMPSDump.xml
    VisItBridgeLAMMPSDump2.xml
    VisItBridgeLAMMPSDumpMultiTime.xml
    VisItBridgeLAMMPSDumpTimeSeries.xml
    VisItBridgeNas.xml
    VisItBridgeNek.xml
    VisItBridgeMoleculeColoring.xml
    VisItBridgePdb.xml
    VisItBridgePFLOTRAN.xml
    # disabling this test for now, not sure why it fails
    # even with 5.7 binarie
    #VisItBridgePixie.xml
    VisItBridgeSamrai.xml
    TruchasReaderWithVisItBridge.xml)
  list(APPEND TESTS_WITH_INLINE_COMPARES
    CONVERGECFDReaderWithVisItBridge.xml
  )
else()
  # A number of VisIt readers support reading of .pdb and
  # of .h5 files, so when the VisItBridge is enabled,
  # the test must take extra steps to select the correct reader.
  #
  # When the VisItBridge is not enabled, there is no need to select
  # a reader.
  list(APPEND TESTS_WITH_BASELINES
    MoleculeColoring.xml
    TruchasReader.xml
    Nek5000Reader.xml)
  list(APPEND TESTS_WITH_INLINE_COMPARES
    CONVERGECFDReader.xml)
endif()

if(PARAVIEW_ENABLE_RAYTRACING)
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
    --script-ignore-output-errors --script $<TARGET_FILE:ParaView::pvbatch> --sym
      ${CMAKE_CURRENT_SOURCE_DIR}/${CATALYST_SIMULATION}
      ${INSITU_SCRIPT} "${duration}"

    --client ${CLIENT_EXECUTABLE}
    --enable-bt
    ${command_line_connect_command}
    --dr
    --test-directory=${PARAVIEW_TEST_DIR}
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${test_name}.xml
    EXTRA_LABELS "catalyst")

  add_pv_test(pvcs "_DISABLE_CS"
    COMMAND
    --script-np 5 --script-ignore-output-errors
    --script $<TARGET_FILE:ParaView::pvbatch> --sym
    ${CMAKE_CURRENT_SOURCE_DIR}/${CATALYST_SIMULATION}
    ${INSITU_SCRIPT} "${duration}"

    --server $<TARGET_FILE:ParaView::pvserver>
    --enable-bt

    --client ${CLIENT_EXECUTABLE}
    --enable-bt
    ${command_line_connect_command}
    --dr
    --test-directory=${PARAVIEW_TEST_DIR}
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${test_name}.xml
    EXTRA_LABELS "catalyst")

endmacro(ADD_CATALYST_LIVE_TEST)

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
  AdaptiveResampleToImage.xml
  AMRArraySelection.xml
  AMRContour.xml
  AMRCutPlane.xml
  AMReXParticlesReader.xml
  AxisAlignedSliceBackwardCompat.xml
  AxisAlignedTransform.xml
  AnimationCache.xml
  AnimationCameraExportImport.xml
  AnimationFollowPath.xml
  AnimationSetTimeCursor.xml
  AnimationUseCamera.xml
  AnisotropyPBR.xml
  Annotations.xml
  ClearCoatPBR.xml
  ConvertToMolecule.xml
  ContourCellData.xml
  CopyPasteFilters.xml
  CopyPasteProperties.xml
  CustomTCoords.xml
  CTHAMRBaseline.xml
  CTHAMRClip.xml
  CTHAMRContour.xml
  CTHAMRDualClip.xml
  CTHAMRMaterialInterfaceFilter.xml
  CTHDerivedDensity2DCylinder.xml
  AddActiveValuesFromVisibleObjects.xml
  AnimatePipelineTime.xml
  AngularPeriodicFilter.xml
  3DWidgetInCustomFilter.xml
  BlockOpacities.xml
  CalcParens.xml
  Calculator.xml
  CalculatorInput.xml
  CameraWidgetViewLink.xml
  CameraWidgetViewLinkState.xml
  CategoriesWithNaN.xml
  CGNSIOSSReader.xml
  # Disabled due to #20508
  # ChangeGlyphRepVisibility.xml
  ChangingTimestepsInStateFiles.xml
  ChartAxisRangeAndLabels.xml
  CheckableHeader.xml
  Clip.xml
  CrinkleClip.xml
  ColorAnnotationsVisibilitiesAndOpacities.xml
  ColorEditorControls.xml
  ColorEditorVolumeControls.xml
  ColorLegendMinMaxLabels.xml
  ColorLegendBackground.xml
  ColorOpacityTableEditing.xml
  ColorOpacityEditorFreehandDrawing.xml
  ColorOpacityEditorRangeHandles.xml
  CombineFrustumSelections.xml
  CategoricalAutomaticAnnotations.xml
  CategoricalColors.xml
  CategoricalOpacities.xml
  ColorByCellDataStringArray.xml
  ComparativeVisPanel.xml
  ComparativeViewOverlay.xml
  # disabling overlay test for now. It has issues with Time 0.
  #  ComparativeOverlay.xml
  CompositeSurfaceSelection.xml
  ComputeConnectedSurfaceProperties.xml
  CustomFilter.xml
  CustomSourceProbe.xml
  CustomViewpoints.xml
  DataOutlines.xml
  DisableResetDisplayEmptyViews.xml
  DualSphereAnimation.xml
  EdgeOpacity.xml
  EdgeTintPBR.xml
  EnSight.xml
  EnSightTensorInversion.xml
  EnSightTensorInversionBin.xml
  ExodusGlobalDataWithRestarts.xml
  ExodusSingleTimeStep.xml
  ExpandSelection.xml
  ExportLinePlotToCSV.xml
  ExportLinePlotToTSV.xml
  ExportSceneSpreadSheetView2.xml
  ExportX3dPOVVRML.xml
  ExTimeseries.xml
  ExTimeseries2.xml
  ExtractCellsByType.xml
  ExtractComponentFilter.xml
  ExtractLevel.xml
  ExtractTimeSteps.xml
  EyeDomeLighting.xml
  FeatureEdges.xml
  FeatureEdgesNonlinearCells.xml
  FFTOverTime.xml
  FieldDataBlocks.xml
  FileDialogOpenFile.xml
  Flagpole.xml
  Flow.xml
  Flow2.xml
  FollowHiddenData.xml
  ForceTime.xml
  Fractal2D.xml
  GenerateTimeSteps.xml
  GenerateTimeStepsData.xml
  GhostCellsGeneratorImageSerial.xml
  Glyph3DCullingAndLOD.xml
  Glyph3DRepresentation.xml
  GlyphUseCellCenters.xml
  GlyphWithEmptyCells.xml
  GroupDataSetOutputType.xml
  glTFImporterNestedRings.xml
  glTFImporterWaterBottle.xml
  glTFReaderAnimatedMorphing.xml
  glTFReaderMultipleAnimations.xml
  glTFReaderMultipleScenes.xml
  glTFReaderToggleDeformation.xml
  glTFReaderTriangle.xml
  H5PartReader.xml
  HideAll.xml
  HorizontalColorLegendTitle.xml
  HyperTreeGridGhostCellsGenerator.xml
  IgnoreLogAxisWarning.xml
  ImageDataToUniformGrid.xml
  ImageVolumeRendering.xml
  InteractiveSelection.xml
  InteractiveViewLinkOffscreen.xml
  InteractiveViewLinkOnscreen.xml
  InteractiveViewLinkState.xml
  LegendGridActor.xml
  LightWidget.xml
  LinearCellExtrusion.xml
  LinesAsTubes.xml
  LinkRepresentationProperties.xml
  LoadPartiallyUpdatedStateFiles.xml
  LoadSaveStateAnimation.xml
  LoadState.xml
  LoadStateChooseDirectory.xml
  LoadStateChooseFiles.xml
  LoadStateTextSourceRepresentation.xml
  LoadStateUseExisting.xml
  LoadSaveStateVolume.xml
  ManyTypesXMLWriterReader.xml
  MatchBoundariesIgnoringCellOrder.xml
  MergePointBlocks.xml
  MergePiecesOnly.xml
  MoleculeBonds.xml
  MoleculeToLines.xml
  # MultiBlockAttributes1.xml
  MultiBlockInspector.xml
  MultiBlockInspectorAssembly.xml
  MultiBlockInspectorAssemblyExtractBlocks.xml
  MultiBlockInspectorAssemblyWithoutSelectOnClickSetting.xml
  MultiBlockInspectorExtractBlocks.xml
  MultiBlockSingleBlock.xml
  MultiBlockVolumeRendering.xml
  MultiBlockInspectorWithoutSelectOnClickSetting.xml
  MultiSliceMultiBlock.xml
  MultiSliceWavelet.xml
  NormalGlyphs.xml
  NumberOfComponentsDomain.xml
  NonConvexPolygon.xml
  OctreeImageFilters.xml
  OMETIFFChannelCalculator.xml
  OpenSaveData.xml
  OrderedGroupDatasets.xml
  OrthographicView.xml
  OutOfRangeColors.xml
  PBRSpheres.xml
  PlotDataOverTime-NonDistributed.xml
  PlotEdges2.xml
  # PlotEdges.xml see paraview/paraview#20659
  PlotGlobalVariablesOverTime.xml
  PlotOverTimeAutoApply.xml
  PolygonCellSelection.xml
  PolygonPointSelection.xml
  PointChartView.xml
  PointGaussianMultiBlockDataSet.xml
  PointGaussianNoScaleTransferFunction.xml
  PointGaussianScaleOpacityArray.xml
  PointInterpolator.xml
  PointVolumeInterpolator.xml
  PolarAxes.xml
  PolarAxesRemoteRendering.xml
  PolarAxes2D.xml
  PolyLineAndSplineSources.xml
  PropertyConversion.xml
  PropertyConversion1.xml
  PropertyConversion2.xml
  PropertyConversion3.xml
  PropertyConversion4.xml
  PropertyLink.xml
  QuadraticGhostCells.xml
  ReadPIOWithHTG.xml
  ReadXMLPolyDataFileSeries.xml
  RecentFiles.xml
  RecentFilesHardFilename.xml
  RectilinearVolumeRendering.xml
  RedistributeDataSetFilter.xml
  RepresentationSelector.xml
  ResampleWithDataset.xml
  RescaleToTemporal.xml
  ReverseLegend.xml
  RotateAroundOriginTransform.xml
  RotateCamera90.xml
  RotationalExtrusionFilter.xml
  # SaveAnimationGeometry.xml # see #20236
  SaveColorMap.xml
  SaveCSV.xml
  SaveFileSeries.xml
  SaveMultiBlockCSV.xml
  SaveOBJ.xml
  SaveTSV.xml
  SaveTXT.xml
  SelectedProxyPanelVisibility.xml
  SelectionBlocks.xml
  SelectionModifiersCells.xml
  SelectionModifiersPoints.xml
  SelectionModifiersBlocks.xml
  SetCustomBasicPresets.xml
  ShaderReplacements.xml
  ShowAll.xml
  SimpleInteraction.xml
  Slice.xml
  SliceAlongPolyline.xml
  SliceAlongSpline.xml
  SolidColorSource.xml
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
  SSAO.xml
  StepColorSpace.xml
  StockColors.xml
  StreamTracerSurface.xml
  StreamTracerUpdates.xml
  TabbedViews.xml
  TensorGlyph.xml
  TemporalInterpolator.xml
  TemporalShiftScale.xml
  TestAnnotateSelectionFilter.xml
  TestBoundaryMeshQuality.xml
  TestGroupDataFromTimeSeries.xml
  TestSelectionOnMultipiece.xml
  TimeStepProgressBar.xml
  Tessellate.xml
  TestOFFReader.xml
  Text2D.xml
  TextBillboard3D.xml
  TextureUsages.xml
  ToneMapping.xml
  TriangleStrips.xml
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
  VolumeIsosurfaceBlendMode.xml
  VolumeRenderingWithContour.xml
  VolumeReprTwoIndepComp.xml
  VolumeSliceBlendMode.xml
  VariableSelector.xml
  VariableSelector1.xml
  ViewSettingsDialog.xml
  VTPSeriesFile.xml
  RandomAttributes.xml
  XYBarChart.xml
  YoungsMaterialInterface.xml
)

if (TARGET VTK::IOParallelLSDyna)
  list(APPEND TESTS_WITH_BASELINES
    Plot3DReader.xml
    SelectReader.xml
    )
endif ()

if (TARGET VTK::IOFides)
  if (PARAVIEW_ENABLE_ADIOS2)
    list(APPEND TESTS_WITH_INLINE_COMPARES
      FidesReaderADIOS2.xml
      FidesWriterADIOS2.xml
      )
  else()
    list(APPEND TESTS_WITH_INLINE_COMPARES
      FidesReader.xml
      FidesWriter.xml
      )
  endif()
endif()

if (TARGET VTK::IOOMF)
  list(APPEND TESTS_WITH_INLINE_COMPARES
    OMFReader.xml
    )
endif()

if(VTK_USE_LARGE_DATA)
  ExternalData_Expand_Arguments(ParaViewData _
    "DATA{${paraview_test_data_directory_input}/Data/bake/bake.e}")
  list(APPEND TESTS_WITH_BASELINES
    RestoreBlockColorDefaultTransferFunction.xml
    )
endif()

if (NOT PARAVIEW_USE_MPI)
  list(APPEND TESTS_WITH_BASELINES
    VolumeCrop.xml
  )
endif()

# VTKHDFWriter does not work with MPI yet
# See issue https://gitlab.kitware.com/vtk/vtk/-/issues/19231
if (TARGET VTK::IOHDF)
  paraview_add_client_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS VTKHDFWriter.xml
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

  PlotMatrixViewArraySelection.xml
  PlotMatrixViewParameters.xml

  # Composite Glyph Trees are only implemented in OpenGL2
  CompositeGlyphTree.xml
  )

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
              --dr
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
         --dr
         --enable-streaming
         --test-directory=${PARAVIEW_TEST_DIR}
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${tname})
  endforeach()
endif()

#------------------------------------------------------------------------------

# Add tests for OSPRay material editor
if (paraview_use_materialeditor)
  list(APPEND TESTS_WITH_INLINE_COMPARES
    MaterialEditorCreateMaterials.xml
    MaterialEditorLoadMaterials.xml
    MaterialEditorSaveMaterials.xml
    MaterialEditorShaderBallScene.xml
  )
endif ()

# Probe picking does not work in render server mode

# Selection is not available in CRS mode
set(BlockContextMenu_DISABLE_CRS TRUE)
set(BlockLinkedSelection_DISABLE_CRS TRUE)
set(MultipleColorOnSelection_DISABLE_CRS TRUE)
set(MultiSliceWavelet_DISABLE_CRS TRUE)
set(MultiSliceMultiBlock_DISABLE_CRS TRUE)
set(PolygonCellSelection_DISABLE_CRS TRUE)
set(PolygonPointSelection_DISABLE_CRS TRUE)
set(NonlinearSubdivisionDisplay_DISABLE_CRS TRUE)
set(InteractiveSelection_DISABLE_CRS TRUE)
set(NonConvexPolygon_DISABLE_CRS TRUE)
set(QuartilePlot_DISABLE_CRS TRUE)
set(TestSelectionOnMultipiece_DISABLE_CRS TRUE)
set(TestHTGSelection_DISABLE_CRS TRUE)
set(SelectionByArray_DISABLE_CRS TRUE) # yes, this test does a selection
set(SelectionBlocks_DISABLE_CRS TRUE)
set(SelectionEditor_DISABLE_CRS TRUE)
set(SelectionModifiersCells_DISABLE_CRS TRUE)
set(SelectionModifiersBlocks_DISABLE_CRS TRUE)
set(SelectionModifiersPoints_DISABLE_CRS TRUE)
set(SelectionLinkBasic_DISABLE_CRS TRUE)
set(SelectionLinkInitial_DISABLE_CRS TRUE)
set(SelectionLinkMultiple_DISABLE_CRS TRUE)
set(SelectionLinkRemove_DISABLE_CRS TRUE)
set(SelectionLinkReaction_DISABLE_CRS TRUE)
set(SelectionLinkScripting_DISABLE_CRS TRUE)
set(TestAnnotateSelectionFilter_DISABLE_CRS TRUE)
set(ExpandSelection_DISABLE_CRS TRUE)
set(ExportSelectionToCSV_DISABLE_CRS TRUE)
set(BlockOpacities_DISABLE_CRS TRUE)
set(SaveExodus_DISABLE_CRS TRUE) # yes, this test does a selection
set(CombineFrustumSelections_DISABLE_CRS TRUE) # yes, this test does a selection
set(RenderViewContextMenu_DISABLE_CRS TRUE)

# BoxWidgetVisibleBlock and ZoomToData are not working as well in crs
set(BoxWidgetVisibleBlock_DISABLE_CRS TRUE)
set(ZoomToData_DISABLE_CRS TRUE)
# These tests cannot be run using ctest -j since they are affected by focus
# changed events.
set (AnimatePipelineTime_FORCE_SERIAL TRUE)
set (AnimateProperty_FORCE_SERIAL TRUE) # since this uses popup window
set (AnimationCache_FORCE_SERIAL TRUE) # since this uses popup window
set (AnimationFollowPath_FORCE_SERIAL TRUE) # since this uses popup window
set (BlockContextMenu_FORCE_SERIAL TRUE) # requires context menu
set (CGNSReaderDataLocation_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (CTHAMRBaseline_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (CTHAMRContour_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (CTHAMRDualClip_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (CTHAMRMaterialInterfaceFilter_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (CTHDerivedDensity2DCylinder_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (Calculator_FORCE_SERIAL TRUE) # since this uses popup-menu
set (CalculatorInput_FORCE_SERIAL TRUE) # since this uses popup-menu
set (CategoricalAutomaticAnnotations_FORCE_SERIAL TRUE) # Seems to works better in serial
set (CategoricalColors_FORCE_SERIAL TRUE) # Uses inline edit popups
set (CategoricalOpacities_FORCE_SERIAL TRUE) # Seems to works better in serial
set (ChartAxisRangeAndLabels_FORCE_SERIAL TRUE) # Uses inline edit popups
set (ColorOpacityTableEditing_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ComparativeViewOverlay_FORCE_SERIAL TRUE) # Seems to works better in serial
set (ComputeArrayMagnitudeSetting_FORCE_SERIAL TRUE) # Seems to works better in serial
set (ConfigurableCategories_FORCE_SERIAL TRUE) # since this uses popup-menu
set (Contour_FORCE_SERIAL TRUE) # since this uses popup-menu
set (CustomViewpoints_FORCE_SERIAL TRUE)
set (DefaultReadersSetting_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (ExodusModeShapes_FORCE_SERIAL TRUE)
set (ExportFilteredColumnsSpreadsheet_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ExpressionChooser_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ExpressionsDialog_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ExpressionsExporter_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ExpressionsSettings_FORCE_SERIAL TRUE) # since this uses combo-box popup-menu
set (LoadSaveStateAnimation_FORCE_SERIAL TRUE)
set (LoadSaveStateVolume_FORCE_SERIAL TRUE)
set (LogColorMapToggle_FORCE_SERIAL TRUE) # since this pops up output window
set (LogColorMap_FORCE_SERIAL TRUE) # since this uses popup-menu
set (MathTextColumn_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (MoleculeToLines_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (PolygonCellSelection_FORCE_SERIAL TRUE)
set (PolygonPointSelection_FORCE_SERIAL TRUE)
set (PropertyLink_FORCE_SERIAL TRUE)
set (PropertyContextMenu_FORCE_SERIAL TRUE)
set (ProxyCategoriesConfiguration_FORCE_SERIAL TRUE) # since this uses popup-line edit
set (ProxyCategoriesToolbar_FORCE_SERIAL TRUE)
set (QuickLaunchCreateProxy_FORCE_SERIAL TRUE)
set (QuickLaunchNavigation_FORCE_SERIAL TRUE)
set (QuickLaunchRequest_FORCE_SERIAL TRUE)
set (ReadPartitionedCGNS_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (RecentFiles_FORCE_SERIAL TRUE) # use key events
set (RecentFilesHardFilename_FORCE_SERIAL TRUE) # use key events
set (SPTimeseries_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (SaveColorMap_FORCE_SERIAL TRUE) # Uses inline edit popups
set (SeriesPresetRegexp_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (SeriesPreset_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (ShaderReplacements_FORCE_SERIAL TRUE)
set (SimpleInteraction_FORCE_SERIAL TRUE) # since this uses popup-menu
set (TestHTGHoverOnCell_FORCE_SERIAL TRUE)  # since this uses QTooltip and QShortcut
set (TestIsoVolume_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (TestPythonView_FORCE_SERIAL TRUE) # Seems to work better in serial.
set (TextSourceBorder_FORCE_SERIAL TRUE) # Seems to work better in serial
set (TimeKeeper_FORCE_SERIAL TRUE) # since this uses popup window
set (TooltipCopy_FORCE_SERIAL TRUE)  # since this uses QTooltip and QShortcut
set (TraceMultiViews_FORCE_SERIAL TRUE) # Seems to works better in serial
set (glTFReaderAnimatedMorphing_FORCE_SERIAL TRUE)  # since this uses popup-menu
set (glTFReaderToggleDeformation_FORCE_SERIAL TRUE)  # since this uses popup-menu

# those tests load and remove macros. Serial avoid side effects
set(MacroEditor_FORCE_SERIAL TRUE)
set(IconBrowser_FORCE_SERIAL TRUE)
set(PythonResetSessionMacro_FORCE_SERIAL TRUE)

# We don't support volume rendering of image data in data server & render server mode
set(AdaptiveResampleToImage_DISABLE_CRS TRUE)
set(LoadSaveStateVolume_DISABLE_CRS TRUE)
set(SeparatedColorMapOpacity_DISABLE_CRS TRUE)
set(VolumeReprTwoIndepComp_DISABLE_CRS TRUE)
set(VolumeCrop_DISABLE_CRS TRUE)
set(VolumeSliceBlendMode_DISABLE_CRS TRUE)
set(VolumeIsosurfaceBlendMode_DISABLE_CRS TRUE)
set(PointVolumeInterpolator_DISABLE_CRS TRUE)

# Set properties for CTH tests
set(CTHAMRContour_DISABLE_CS TRUE)
set(CTHAMRContour_DISABLE_CRS TRUE)
set(CTHAMRDualClip_DISABLE_CS TRUE)
set(CTHAMRDualClip_DISABLE_CRS TRUE)
set(CTHAMRClip_DISABLE_CS TRUE)
set(CTHAMRClip_DISABLE_CRS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CRS TRUE)

# Since the test uses surface-selection, it cannot work in render-server mode.
set(XYChart_DISABLE_CRS TRUE)
# this uses charts: nb of points changes in client server, so graphs differs
set(LinkViews_DISABLE_CS TRUE)
set(LinkViews_DISABLE_CRS TRUE)

# Test needs to load a plugin, the test only records that for the
# client side.
set (SteeringDataGenerator_DISABLE_CS TRUE)
set (SteeringDataGenerator_DISABLE_CRS TRUE)

set (PlotOverTimeAutoApply_DISABLE_CRS TRUE) # since this uses surface selection.
# The ExportLinePlotToTSV test uses the same image baseline as the ExportLinePlotToCSV test
set (ExportLinePlotToTSV_BASELINE ExportLinePlotToCSV.png)

# ExportSceneSpreadSheetView2 won't work in parallel CS because of ghost cells
# than cannot be processed by TableToStructuredGrid.
set(ExportSceneSpreadSheetView2_DISABLE_CS TRUE)
set(ExportSceneSpreadSheetView2_DISABLE_CRS TRUE)

# Cannot support CRS since we do volume rendering of image data
# in this test.
set (RemoteRendering_DISABLE_CRS TRUE)
set (VolumeRenderingWithContour_DISABLE_CRS TRUE)
set (VolumeNoMapScalars_DISABLE_CRS TRUE)
set (MultiBlockVolumeRendering_DISABLE_CRS TRUE)
set (ColorOpacityEditorFreehandDrawing_DISABLE_CRS TRUE)

# Composite Surface Selection is currently broken in everything but builtin
SET (CompositeSurfaceSelection_DISABLE_CS TRUE)
SET (CompositeSurfaceSelection_DISABLE_CRS TRUE)

# Clip test has object picking which is not supported in client-render-server
# mode.
SET (Clip_DISABLE_CRS TRUE)

# Image and rectilinear grid volume rendering not supported in CRS mode.
set (ImageVolumeRendering_DISABLE_CRS TRUE)
set (RectilinearVolumeRendering_DISABLE_CRS TRUE)

# The SaveTXT and SaveTSV tests use the same image baseline as the SaveCSV test
SET (SaveTSV_BASELINE SaveCSV.png)
SET (SaveTXT_BASELINE SaveCSV.png)

# Disable some testing configurations for these tests.

## Disable ClientRenderServer tests for FFTOverTime. This is done since
## selection is not supported in render server mode esp. when number of render
## server processes is not the same as the data server processes
set (FFTOverTime_DISABLE_CRS TRUE)

# ColorEditorVolumeControls does volume rendering of structured data which required remote
# rendering in client-serve mode.
SET (ColorEditorVolumeControls_DISABLE_CS TRUE)
SET (ColorEditorVolumeControls_DISABLE_CRS TRUE)

# ViewSettingsDialog, TextureUsages, LogoSourcesInChartViews and LogoSource use texture which are not supported in pvcrs mode.
SET (ViewSettingsDialog_DISABLE_CRS TRUE)
SET (TextureUsages_DISABLE_CRS TRUE)
SET (LogoSourcesInChartViews_DISABLE_CRS TRUE)
SET (LogoSource_DISABLE_CRS TRUE)

# MultiBlockAttributes1 requires selection which doesn't work on pvcrs
SET (MultiBlockAttributes1_DISABLE_CRS TRUE)

# Due to issue #20657, the tangents are not sent in pvcrs mode
# event using remote rendering
# We can remove this when issue #20657 is resolved
SET (AnisotropyPBR_DISABLE_CRS TRUE)

## Disable ClientRenderServer tests for TransferFunction2D. This is done since
## image data delivery is not supported in render server mode
SET (TransferFunction2D_DISABLE_CRS TRUE)
SET (TransferFunction2DYScalars_DISABLE_CRS TRUE)
SET (TransferFunction2DYScalarsEditor_DISABLE_CRS TRUE)
SET (TransferFunction2DYScalarsEditorA_DISABLE_CRS TRUE)
SET (TransferFunction2DYScalarsEditorB_DISABLE_CRS TRUE)

# Plugins are only built as shared libraries.
IF (NOT BUILD_SHARED_LIBS)
  SET (NiftiReaderWriterPlugin_DISABLE_C TRUE)
ENDIF ()
# There should be a client server specific version of this test.
SET (NiftiReaderWriterPlugin_DISABLE_CS TRUE)
SET (NiftiReaderWriterPlugin_DISABLE_CRS TRUE)

# Make these tests use reverse connection.
SET (CutMulti_REVERSE_CONNECT TRUE)

# Selection not supported in CRS and is needed for this test.
set (ResetToVisibleRange_DISABLE_CRS TRUE)

# Molecules are not supported in client-serve modes, currently.
#set (Molecule_DISABLE_CS TRUE)
#set (Molecule_DISABLE_CRS TRUE)
#set (MoleculeColoring_DISABLE_CS TRUE)
#set (MoleculeColoring_DISABLE_CRS TRUE)

# some glTF tests are not working in CS currently
# https://gitlab.kitware.com/paraview/paraview/-/issues/20572
set (glTFReaderToggleDeformation_DISABLE_CS TRUE)
set (glTFReaderToggleDeformation_DISABLE_CRS TRUE)
set (glTFReaderMultipleScenes_DISABLE_CS TRUE)
set (glTFReaderMultipleScenes_DISABLE_CRS TRUE)
set (glTFReaderMultipleAnimations_DISABLE_CS TRUE)
set (glTFReaderMultipleAnimations_DISABLE_CRS TRUE)

# pvd writer/reader has issues creating directories or filenames with punctuation
# https://gitlab.kitware.com/paraview/paraview/-/issues/22361
if (NOT APPLE)
  set (RecentFilesHardFilename_DISABLE_CS TRUE)
  set (RecentFilesHardFilename_DISABLE_CRS TRUE)
endif()

# tests involve pointCount checks, so not reliable in CS modes.
set (PlotOverLine_3d_DISABLE_CS TRUE)
set (PlotOverLine_3d_DISABLE_CRS TRUE)
set (PlotOverLine_htg_DISABLE_CS TRUE)
set (PlotOverLine_htg_DISABLE_CRS TRUE)
set (PlotOverLine_surface_DISABLE_CS TRUE)
set (PlotOverLine_surface_DISABLE_CRS TRUE)

# CGNS BC and Surface patches not supported in parallel
set(ReadCGNSBCDataset_DISABLE_CS TRUE)
set(ReadCGNSBCDataset_DISABLE_CRS TRUE)
set(CONVERGECFDCGNSReader_DISABLE_CS TRUE)
set(CONVERGECFDCGNSReader_DISABLE_CRS TRUE)
set(CGNSReaderSurfacePatches_DISABLE_CS TRUE)
set(CGNSReaderSurfacePatches_DISABLE_CRS TRUE)

# RegionIds is not implemented for Distributed context
set (RegionIds_DISABLE_CS TRUE)
set (RegionIds_DISABLE_CRS TRUE)

# AxisAlignedTransform is not implemented for Distributed context yet : issue (https://gitlab.kitware.com/paraview/paraview/-/issues/22949)
set (AxisAlignedTransform_DISABLE_CS TRUE)
set (AxisAlignedTransform_DISABLE_CRS TRUE)

# Add image method overrides for tests.

# images with text need loose method
set(4AxesContextView_METHOD LOOSE_VALID)
set(BoxClipStateBackwardsCompatibility_METHOD LOOSE_VALID)
set(ChartAxisRangeAndLabels_METHOD LOOSE_VALID)
set(ChartLoadNoVariables_METHOD LOOSE_VALID)
set(ColorOpacityEditorRangeHandles_METHOD LOOSE_VALID)
set(ExodusModeShapes_METHOD LOOSE_VALID)
set(ExportLinePlotToCSV_METHOD LOOSE_VALID)
set(ExportLinePlotToTSV_METHOD LOOSE_VALID)
set(HistogramSelection_METHOD LOOSE_VALID)
set(LineChartSelection_METHOD LOOSE_VALID)
set(OrthographicView_METHOD LOOSE_VALID)
set(PlotGlobalVariablesOverTime_METHOD LOOSE_VALID)
set(PointChartView_METHOD LOOSE_VALID)
set(PolarAxesRemoteRendering_METHOD LOOSE_VALID)
set(Preview_METHOD LOOSE_VALID)
set(ProgrammableAnnotation_METHOD LOOSE_VALID)
set(ReloadExodusFile_METHOD LOOSE_VALID)
set(SaveLargeScreenshot_METHOD LOOSE_VALID)
set(SeriesPresetRegexp_METHOD LOOSE_VALID)
set(SimpleInteraction_METHOD LOOSE_VALID)
set(SortLineChartData_METHOD LOOSE_VALID)
set(SpreadSheet2_METHOD LOOSE_VALID)
set(TileDisplaySplitView-1x1_METHOD LOOSE_VALID)
set(TileDisplaySplitView-2x1_METHOD LOOSE_VALID)
set(TileDisplaySplitView-2x2_METHOD LOOSE_VALID)
set(TileDisplaySplitView-3x1_METHOD LOOSE_VALID)
set(TransferFunction2DEditor_METHOD LOOSE_VALID)
set(TransferFunction2DYScalars_METHOD LOOSE_VALID)
set(XYChart_METHOD LOOSE_VALID)
set(XYHistogram_METHOD LOOSE_VALID)

set(TESTS_WITH_MULTI_SERVERS_3
  TestMultiServer3.xml
)

# ResampleToHyperTreeGrid only works in client mode
paraview_add_client_tests(
  TEST_SCRIPTS ResampleToHyperTreeGrid.xml ResampleImageToHyperTreeGrid.xml
)

# RandomAttributes generate different results depending on the data
# distribution so we enable it on client mode only for now
# See https://gitlab.kitware.com/paraview/paraview/-/issues/22596
paraview_add_client_tests(
  TEST_SCRIPTS RandomAttributesHTG.xml
)

# FDS Reader has issues in client-server mode.
# See https://gitlab.kitware.com/paraview/paraview/-/issues/22687
paraview_add_client_tests(
  TEST_SCRIPTS TestFDSReader.xml
)

# TODO: remote rendering tests and reverse connect tests.

paraview_add_multi_server_tests(3
   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
   TEST_SCRIPTS ${TESTS_WITH_MULTI_SERVERS_3})

paraview_add_tile_display_tests(1 1
   TEST_SCRIPTS TileDisplaySplitView.xml)
paraview_add_tile_display_tests(1 1
   TEST_SCRIPTS LinkCameraFromView.xml)
if (PARAVIEW_USE_MPI)
  paraview_add_tile_display_tests(2 1
     TEST_SCRIPTS TileDisplaySplitView.xml)
  paraview_add_tile_display_tests(2 2
     TEST_SCRIPTS TileDisplaySplitView.xml)
  paraview_add_tile_display_tests(3 1
     TEST_SCRIPTS TileDisplaySplitView.xml)
endif()

paraview_add_cave_tests(2 "${CMAKE_CURRENT_SOURCE_DIR}/LeftRight.pvx"
  TEST_SCRIPTS TestCAVE.xml TestCAVEVolumeRendering.xml)

#------------------------------------------------------------------
# Add tests that test command line arguments (among other things).
#------------------------------------------------------------------
if (TARGET ParaView::paraview)
  # The state file need to point to the correct data file. We do that by
  # configuring the state file.
  paraview_add_client_tests(
    ARGS "--state=${CMAKE_CURRENT_SOURCE_DIR}/exodusStateFile.3.14.1.pvsm"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS LoadExodusStateFile.xml
    ENVIRONMENT
      PARAVIEW_DATA_ROOT=${paraview_test_data_directory_output}
  )

  paraview_add_client_tests(
    ARGS "--state=${CMAKE_CURRENT_SOURCE_DIR}/boxClip.5.6.pvsm"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS BoxClipStateBackwardsCompatibility.xml)

  paraview_add_client_tests(
    ARGS "--data=${paraview_test_data_directory_output}/Testing/Data/FileSeries/blow..vtk"
    TEST_SCRIPTS CheckDataArgumentsWithFileSeries.xml
  )

  paraview_add_client_tests(
    ARGS " ${paraview_test_data_directory_output}/Testing/Data/FileSeries/blow..vtk"
    TEST_SCRIPTS CheckPositionalArgumentsWithFileSeries.xml
  )

  if (PARAVIEW_USE_PYTHON)
    # Test whether or not we can load a Python state file with the --state
    # command line option as well as loading a Python state file that was
    # generated with PV 5.4.1. Loading the 5.4.1 Python state file checks
    # on backwards compatibility.
    paraview_add_client_tests(
      ARGS "--state=${CMAKE_CURRENT_SOURCE_DIR}/Calculator54State.py"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS Calculator54State.xml)
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
# Very small texture size limits on mesa cause error messages for some tests that
# include coloring by cell data. Let's skip those.
set(texture_buffer_exceeding_hardware_tests
  CONVERGECFDReader
  CONVERGECFDReaderWithVisItBridge
  TestGroupDataFromTimeSeries
  OMFReader
)

foreach(test_name IN LISTS texture_buffer_exceeding_hardware_tests)
  foreach(prefix IN ITEMS pv pvcs pvcrs)
    if (TEST "${prefix}.${test_name}")
      set_property(TEST "${prefix}.${test_name}" APPEND PROPERTY
        SKIP_REGULAR_EXPRESSION "Attempt to use a texture buffer exceeding your hardware")
    endif()
  endforeach()
endforeach()

# Server Connection specific tests

if(NOT WIN32) # On windows, QtTesting is not able to press the connection dialog buttons
  paraview_add_client_tests(
    SMTESTING_ALLOW_ERRORS
    FORCE_SERIAL
    TEST_SCRIPTS ServerConnectTimeout.xml)
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/configuredServer.pvsc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/configuredServer.pvsc" @ONLY)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/ServerConnectConfigured.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ServerConnectConfigured.xml" @ONLY)

paraview_add_client_tests(
    FORCE_SERIAL
    TEST_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/ServerConnectConfigured.xml)

if (PARAVIEW_PLUGIN_ENABLE_ArrowGlyph)
  configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/ServerConnectPluginLoad.xml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ServerConnectPluginLoad.xml" @ONLY)

  paraview_add_client_tests(
    FORCE_SERIAL
    TEST_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/ServerConnectPluginLoad.xml)
endif ()

# PropertyPanelVisibilitiesOverride test requires a configuration file.
# Use a custom TEST_DIRECTORY for that: this overrides user config dir when testing
# (see pqCoreUtilities::getParaViewUserDirectory). Copy config in it.
set (PropertyPanelVisibilitiesOverrideDir "${CMAKE_CURRENT_BINARY_DIR}/PropertyPanelVisibilitiesOverride")
file (MAKE_DIRECTORY ${PropertyPanelVisibilitiesOverrideDir})
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanelVisibilities.json.in"
  "${PropertyPanelVisibilitiesOverrideDir}/PropertyPanelVisibilities.json" @ONLY)
paraview_add_client_tests(
  TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanelVisibilitiesOverride.xml
  TEST_DIRECTORY ${PropertyPanelVisibilitiesOverrideDir}
)
