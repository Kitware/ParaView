
list(APPEND TESTS_WITH_BASELINES
  # Test uses D3 which is available in MPI only builds.
  D3SmallCells.xml
  EDLWithSubsampling.xml
  # Test uses D3 which is available in MPI only builds.
  ExportSelectionToCSV.xml
  # This test check spreadsheet values which include processID
  ManyTypesXMLWriterReaderMPI.xml
  # Test Use Data Partitions for volume rendering
  #UseDataPartitions.xml
  )

list(APPEND TESTS_WITH_INLINE_COMPARES
  # needs parallel server for the columns to match those in the test.
  SpreadSheetParallelData.xml)

# test only checks for spreadsheet columns correctly in
# parallel client-server mode.
set (SpreadSheetParallelData_DISABLE_C TRUE)

# DistributePoints is only tested in non-built-in mode.
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS DistributePoints.xml
  )
paraview_add_client_server_render_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS DistributePoints.xml
  )

# Process IDs tests are only tested in non-built-in mode.
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GenerateProcessIds.xml ProcessIdsHTG.xml
  )
paraview_add_client_server_render_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GenerateProcessIds.xml ProcessIdsHTG.xml
  )

# Global IDs tests are only tested in non-built-in mode.
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GlobalPointAndCellIdsHTG.xml
  )
paraview_add_client_server_render_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GlobalPointAndCellIdsHTG.xml
  )

# Volume rendering seems to work better in serial
set (UseDataPartitions_FORCE_SERIAL TRUE)
# This test requires a parallel partition to make sense so we
# skip running it with the built-in server.
paraview_add_client_server_tests(
  TEST_SCRIPTS UseDataPartitions.xml
  )

# Test a bug (#22337) that could only happen in non built-in mode
# This test has no baseline.
paraview_add_client_server_tests(
  TEST_SCRIPTS POpenFoamUpdate.xml
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

# HTG Source has special features when used in an MPI setting
paraview_add_client_server_tests(
  TEST_SCRIPTS HyperTreeGridSourceDistributed.xml
  )

# GhostCellGeneratorSynchronize is only tested in non-built-in mode.
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GhostCellsGeneratorSynchronize.xml
  )
paraview_add_client_server_render_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GhostCellsGeneratorSynchronize.xml
  )

# GhostCellsGeneratorImageDistributed needs specifically two servers
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GhostCellsGeneratorImageDistributed.xml
  NUMSERVERS 2
  )
paraview_add_client_server_render_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS GhostCellsGeneratorImageDistributed.xml
  NUMSERVERS 2
  )
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS HyperTreeGridGhostCellsGeneratorComposite.xml
  NUMSERVERS 2
  )

# AppendReduceFilter needs specifically four servers to generate duplicate points
paraview_add_client_server_tests(
  TEST_SCRIPTS vtkAppendReduceFilter.xml
  NUMSERVERS 4
)

paraview_add_client_server_tests(
  TEST_SCRIPTS GhostFeatureEdgesAndWireframe.xml
  )
paraview_add_client_server_render_tests(
  TEST_SCRIPTS GhostFeatureEdgesAndWireframe.xml
  )

paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS RemoveGhostInterfaces.xml
  )
paraview_add_client_server_render_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS RemoveGhostInterfaces.xml
  )

# Regression test for https://gitlab.kitware.com/paraview/paraview/-/issues/21396
# Opacity rendering specifically needs to be tested with >4 procs and remote rendering
paraview_add_client_tests(
  TEST_SCRIPTS TestOpacityRendering.xml
)
paraview_add_client_server_tests(
  TEST_SCRIPTS TestOpacityRendering.xml
  NUMSERVERS 5
)
paraview_add_client_server_render_tests(
  TEST_SCRIPTS TestOpacityRendering.xml
  NUMSERVERS 5
)

# Regression test for https://gitlab.kitware.com/paraview/paraview/-/issues/22681
# as it occurs when we have >=4 procs
paraview_add_client_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS VolumeCrop.xml
)
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS VolumeCrop.xml
  NUMSERVERS 4
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
set(PointVolumeInterpolator_DISABLE_CS TRUE)

# Test uses distributed sources
set(GhostCellsGeneratorImageSerial_DISABLE_CS TRUE)
set(GhostCellsGeneratorImageSerial_DISABLE_CRS TRUE)

# The following tests are only run in client-server mode.
# They are needed to verify the remote saving/loading of different files, such as states and screenshots, work.
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS SaveLoadRemoteState.xml
  )
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS SaveRemoteScreenshot.xml
  )
paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS SaveRemoteAnimation.xml
  )

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

# For the same reasons
set (MultiBlockChartSelection_DISABLE_CS TRUE)
set (MultiBlockChartSelection_DISABLE_CRS TRUE)

# Selection link may highlight incorrect ID in parallel,
# hence disable them.
set(SelectionLinkBasic_DISABLE_CS TRUE)
set(SelectionLinkInitial_DISABLE_CS TRUE)
set(SelectionLinkMultiple_DISABLE_CS TRUE)
set(SelectionLinkRemove_DISABLE_CS TRUE)
set(SelectionLinkReaction_DISABLE_CS TRUE)
set(SelectionLinkScripting_DISABLE_CS TRUE)

# Many types has a MPI version and Sequential version
set(ManyTypesXMLWriterReader_DISABLE_CS TRUE)
set(ManyTypesXMLWriterReader_DISABLE_CRS TRUE)
set(ManyTypesXMLWriterReaderMPI_DISABLE_C TRUE)
