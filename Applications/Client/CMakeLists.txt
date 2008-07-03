PROJECT(pqClient)

MACRO(GET_IMAGE_THRESHOLD_ARG variable testname)
# Macro used to obtain the command argument to set the image threshold.
  SET (${variable})
  IF (${testname}_THRESHOLD)
    SET (${variable} --image-threshold=${${testname}_THRESHOLD})
  ENDIF (${testname}_THRESHOLD)
ENDMACRO(GET_IMAGE_THRESHOLD_ARG)

MACRO(GET_REVERSE_CONNECT_ARG variable testname)
# Macro used to obtain when this test must use reverse connection
  SET (${variable})
  IF (${testname}_REVERSE_CONNECT)
    SET (${variable} --test-rc)
  ENDIF (${testname}_REVERSE_CONNECT)
ENDMACRO(GET_REVERSE_CONNECT_ARG)

SET(QT_USE_QTASSISTANT TRUE)
SET(QT_USE_QTNETWORK TRUE) # QtAssistant depends on it (only for linking)
SET(QT_USE_QTUITOOLS TRUE)
INCLUDE(${QT_USE_FILE})

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

INCLUDE_DIRECTORIES(
  ${pqClient_SOURCE_DIR}
  ${pqClient_BINARY_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  AboutDialog.h
  MainWindow.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  AboutDialog.ui
  MainWindow.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
  Resources.qrc
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)

IF(WIN32)
  SET(EXE_ICON pqClient.rc)
ENDIF(WIN32)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
  ${EXE_ICON}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

IF(WIN32)
  LINK_DIRECTORIES(${QT_LIBRARY_DIR})
ENDIF(WIN32)

IF(Q_WS_MAC)
  SET(apple_bundle_sources "MacIcon.icns")
  SET_SOURCE_FILES_PROPERTIES(
    "MacIcon.icns"
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
    )
  SET(MACOSX_BUNDLE_ICON_FILE MacIcon.icns)
  SET(MAKE_BUNDLE MACOSX_BUNDLE)
ENDIF(Q_WS_MAC)

# Add main server executables.
IF(NOT Q_WS_MAC)
  SET(PV_EXE_LIST paraview)
ENDIF(NOT Q_WS_MAC)
# Add Qt assistant to the list of shared lib. forwarded executables.
# If this is enabled, the build system will create an executable
# call assistant. This executable will set the appropriate ld library
# path and execute assistant-real. It is the installation's requirement
# to install assistant-real
SET(PV_EXE_LIST ${PV_EXE_LIST} assistant)

SET(MACOSX_BUNDLE_BUNDLE_NAME "ParaView")
IF(NOT Q_WS_MAC)
  SET(PV_EXE_NAME paraview${PV_EXE_SUFFIX})
ELSE(NOT Q_WS_MAC)
  SET(PV_EXE_NAME paraview)
ENDIF(NOT Q_WS_MAC)

ADD_EXECUTABLE(${PV_EXE_NAME} WIN32 ${MAKE_BUNDLE}  
  AboutDialog.cxx
  AboutDialog.h
  MainWindow.cxx
  MainWindow.h
  main.cxx
  ProcessModuleGUIHelper.cxx
  ProcessModuleGUIHelper.h
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
  ${EXE_ICON}
  ${apple_bundle_sources}
)

TARGET_LINK_LIBRARIES(${PV_EXE_NAME}
  pqComponents
  pqCore
  pqWidgets
  QtChart
  QtTesting
  ${QT_LIBRARIES}
)

IF(Q_WS_MAC)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CompleteBundle.cmake"
    @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CreateBundle.sh"
    @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundleDuringMakeInstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CreateBundleDuringMakeInstall.cmake"
    @ONLY IMMEDIATE)
ENDIF(Q_WS_MAC)

IF(WIN32)
  TARGET_LINK_LIBRARIES(${PV_EXE_NAME} ${QT_QTMAIN_LIBRARY} )
ENDIF(WIN32)

EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_BINS"
             OUTPUT_VARIABLE PV_ASSISTANT)
FILE(TO_NATIVE_PATH "${PV_ASSISTANT}/" PV_ASSISTANT)
FILE(TO_NATIVE_PATH "${ParaView_BINARY_DIR}/pqClient.adp" PV_CLIENT_DOC)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqClientDocFinder.txt.in"
               "${EXECUTABLE_OUTPUT_PATH}/pqClientDocFinder.txt"
               @ONLY IMMEDIATE)
  
IF(BUILD_TESTING)
  IF(Q_WS_MAC)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview --test-directory=${PARAVIEW_TEST_DIR})
  ELSE(Q_WS_MAC)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
  ENDIF(Q_WS_MAC)

  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_PYTHON ${pqClient_SOURCE_DIR}/Testing/Python)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  SET (XML_TESTS_WITH_BASELINES
    # ColorEditor3 doesn't work in client/server mode
    ColorEditor3
    CreateSourceWithoutConnect
#    LoadStateSelection
    OpenFile
#    SelectionUndo
    )

  SET (TESTS_WITHOUT_BASELINES
    AboutDialog
    ChartOptions
    )
  IF(PARAVIEW_DATA_ROOT)
    SET (TESTS_WITHOUT_BASELINES
      ${TESTS_WITHOUT_BASELINES}
      LoadStateHistogram
      )
  ENDIF(PARAVIEW_DATA_ROOT)

  IF(QT_TESTING_WITH_PYTHON)
    SET(XML_TESTS_WITH_BASELINES ${XML_TESTS_WITH_BASELINES}
      PythonProgrammableFilter
      )
    SET(PYTHON_TESTS
      OpenHelp
      ToolBarAndMenus
      TestContourWidget
      )
    IF(PARAVIEW_DATA_ROOT)
      SET(PYTHON_TESTS ${PYTHON_TESTS}
        #HistogramChart
        LoadStateMultiView
        FileSeries
        SaveLargeScreenshot
        #SaveAnimationMultiView   -- need to fix race condition on Mac
        VCRAnimation
        )
      # Set image threshold overrides for the tests.
      SET(SaveLargeScreenshot_THRESHOLD 10000)

      SET(PYTHON_TESTS_REMOTE_ONLY 
        DisconnectAndSaveAnimation)
    ENDIF(PARAVIEW_DATA_ROOT)
  ENDIF(QT_TESTING_WITH_PYTHON)

  FOREACH(test ${TESTS_WITHOUT_BASELINES})
    ADD_TEST(pqClient${test}
      ${TEST_BINARY} -dr 
        --test-directory=${PARAVIEW_TEST_DIR}
        --run-test=${TEST_XML}/${test}.xml --exit)
  ENDFOREACH(test)
  
  IF(PARAVIEW_DATA_ROOT)
    FOREACH(test ${XML_TESTS_WITH_BASELINES})
      GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
      ADD_TEST(pqClient${test}
        ${TEST_BINARY} -dr --run-test=${TEST_XML}/${test}.xml
        --test-directory=${PARAVIEW_TEST_DIR}
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
    ENDFOREACH(test) 
  ENDIF(PARAVIEW_DATA_ROOT)
  
  FOREACH(test ${PYTHON_TESTS})
    GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
    ADD_TEST(pqClient${test}-Python
      ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
      --test-directory=${PARAVIEW_TEST_DIR}
      ${THRESHOLD_CMD}
      --exit)
  ENDFOREACH(test)

  # test the plugins
  IF(BUILD_SHARED_LIBS AND BUILD_EXAMPLES)
    SET(DOLLAR "$")
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE}/paraview --test-directory=${PARAVIEW_TEST_DIR})
      SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE})
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH})
      IF(Q_WS_MAC)
        SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview --test-directory=${PARAVIEW_TEST_DIR})
      ELSE(Q_WS_MAC)
        SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
      ENDIF(Q_WS_MAC)
    ENDIF(CMAKE_CONFIGURATION_TYPES)
  

    IF (QT_TESTING_WITH_PYTHON)
      # Since these tests need python, it's essential to verify that testing
      # with python is enabled.
      SET(PLUGIN_TESTS
          ToolBarPlugin
          ObjectPanelPlugin
          )

      FOREACH(test ${PLUGIN_TESTS})
        ADD_TEST(pqClient${test}
                 ${CMAKE_COMMAND} -E chdir ${RUN_PATH}
                 ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
                 --exit)
      ENDFOREACH(test)
    ENDIF (QT_TESTING_WITH_PYTHON)

  ENDIF(BUILD_SHARED_LIBS AND BUILD_EXAMPLES)


# Now we add tests that use the pvTestDriver for running.
  SET(XML_TESTS_WITH_BASELINES
    3DWidgetInCustomFilter
    AnimatePipelineTime
    CalcParens
    Calculator
    CameraLink
    Clip
    ClipHidden
    ClipPlaneLink
    ClipUndo
    ColorEditor1
    ColorEditor2
    ColorEditor4
    Contour
    ContourRange
    CreateAndLoadLookmark
    CreateCustomFilter
    CreateSource
    CustomFilter
    Cut
    CutMulti
    D3SmallCells
    DualSphereAnimation
    ExodusIIPanel
    ExodusModeShapes
    ExodusPanel
    ExodusWriter
    ExodusXML
    ExTimeseries
    ExtractBlock
    ExtractLevel
    Flow
    Flow2
    Fractal2D
    Fractal3D
    LinePlot
    LoadSaveStateAnimation
    LoadState
    ObjectInspectorSphere
    OpenSaveData
    OrientationAxis
    Plot3DReader
    PropertyLink
    RectilinearFractal
    RepresentationSelector
    SaveAnimationGeometry
    SaveColorMap
    SelectReader
    SimpleInteraction
    Slice
    SPTimeseries
    StreamTracer
    StreamTracerUpdates
    TemporalInterpolator
    TemporalShiftScale
    Tessellate
    TextWidget
    Threshold
    UndoRedo
    UndoRedo1
    UndoRedo2
    UndoRedo3
    UndoRedo4
    UndoRedo5
    UndoRedo6
    UndoRedo7
    UnstructuredOutline
    VariableSelector
    ViewSettingsDialog
    XdmfRead
    XdmfReadImageData
    XdmfReadImageDataCollection
    XdmfReadRectilinearGrid
    XdmfReadRectilinearGridCollection
    XdmfReadStructuredGrid
    XdmfReadStructuredGridCollection
  )

  # Add image threshold overrides for tests.
  # Generally all tests with wireframes need higher thresholds.
  SET (Flow2_THRESHOLD 15)
  SET (UndoRedo4_THRESHOLD 15)

  # These Xdmf tests have wireframes, hence the increased thresholds.
  SET (XdmfReadImageData_THRESHOLD 20)
  SET (XdmfReadRectilinearGrid_THRESHOLD 20)
  SET (XdmfReadRectilinearGridCollection_THRESHOLD 20)
  SET (XdmfReadStructuredGrid_THRESHOLD 20)
  SET (XdmfReadStructuredGridCollection_THRESHOLD 20)
  SET (XdmfReadImageDataCollection_THRESHOLD 20)

  # Make these tests use reverse connection.
  SET (CutMulti_REVERSE_CONNECT TRUE)

  # Test compositing for a few client server and client render server tests.
  SET(COMPOSITING_XML_TESTS_WITH_BASELINES
    Calculator
    CameraLink
    ColorEditor1
    ContourRange
    D3SmallCells
    Fractal3D
    StreamTracer
    UndoRedo2
    )

  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver)

  IF(PARAVIEW_DATA_ROOT)
    OPTION(PARAVIEW_CLIENT_RENDER_SERVER_TESTS
      "Turn on/off client tests that involve separate data and render servers."
      ON)
    MARK_AS_ADVANCED(PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    FOREACH(test ${XML_TESTS_WITH_BASELINES})
      GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
      GET_REVERSE_CONNECT_ARG(REVERSE_CONNECTION_CMD ${test})

      ADD_TEST(pqClient${test}-Builtin
        ${TEST_BINARY}
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
      
      ADD_TEST(pqClient${test}-ClientServer
        ${TEST_BINARY} --test-server
        ${REVERSE_CONNECTION_CMD}
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)

      IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
        ADD_TEST(pqClient${test}-ClientRenderServer
          ${TEST_BINARY} --test-render-server
          ${REVERSE_CONNECTION_CMD}
          --test-directory=${PARAVIEW_TEST_DIR}
          -dr
          --run-test=${TEST_XML}/${test}.xml
          --compare-view=${TEST_IMAGE}/${test}.png
          ${THRESHOLD_CMD}
          --exit)
      ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test)

    FOREACH(test ${COMPOSITING_XML_TESTS_WITH_BASELINES})
      GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
      ADD_TEST(pqClient${test}-ClientServerRemoteRender
        ${TEST_BINARY} --test-server --test-remote-rendering
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
        
      IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
        ADD_TEST(pqClient${test}-ClientRenderServerRemoteRender
          ${TEST_BINARY} --test-render-server --test-remote-rendering
          --test-directory=${PARAVIEW_TEST_DIR}
          -dr
          --run-test=${TEST_XML}/${test}.xml
          --compare-view=${TEST_IMAGE}/${test}.png
          ${THRESHOLD_CMD}
          --exit)
      ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test)

    FOREACH(test ${PYTHON_TESTS_REMOTE_ONLY})
        GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
        ADD_TEST(pqClient${test}-ClientServer-Python
          ${TEST_BINARY} --test-server
          --test-directory=${PARAVIEW_TEST_DIR}
          -dr
          --run-test=${TEST_PYTHON}/${test}.py
          ${THRESHOLD_CMD}
          --exit)

     #FIXME
     #IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
     #  ADD_TEST(pqClient${test}-ClientRenderServer-Python
     #    ${TEST_BINARY} --test-render-server
     #    --test-directory=${PARAVIEW_TEST_DIR}
     #    -dr
     #    --run-test=${TEST_PYTHON}/${test}.py
     #    --exit)
     #ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test) 

    ADD_TEST(StructuredVolumeRender-ClientServerRemoteRender
      ${TEST_BINARY} --test-server --test-remote-rendering
      --test-directory=${PARAVIEW_TEST_DIR}
      -dr
      --run-test=${TEST_XML}/StructuredVolumeRender.xml
      --compare-view=${TEST_IMAGE}/StructuredVolumeRender.png
      --exit)
    ADD_TEST(UnstructuredVolumeRender-ClientServerRemoteRender
      ${TEST_BINARY} --test-server --test-remote-rendering
      --test-directory=${PARAVIEW_TEST_DIR}
      -dr
      --run-test=${TEST_XML}/UnstructuredVolumeRender.xml
      --compare-view=${TEST_IMAGE}/UnstructuredVolumeRender.png
      --exit)

  ENDIF(PARAVIEW_DATA_ROOT)

  ADD_SUBDIRECTORY(Testing/Cxx)

  # Do the link forwarding for the test driver as well.
  SET(PV_EXE_LIST ${PV_EXE_LIST} pvTestDriver)
ENDIF(BUILD_TESTING)

# Add shared link forwarding executables if necessary.
IF(PV_NEED_SHARED_FORWARD)
  FOREACH(pvexe ${PV_EXE_LIST})
    SET(PV_FORWARD_EXE ${pvexe}${PV_EXE_SUFFIX})
    CONFIGURE_FILE(
      ${ParaView_SOURCE_DIR}/Servers/Executables/pv-forward.c.in
      ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c
      @ONLY IMMEDIATE)
    ADD_EXECUTABLE(${pvexe} ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c)
    ADD_DEPENDENCIES(${pvexe} ${pvexe}${PV_EXE_SUFFIX})
    INSTALL(TARGETS ${pvexe} DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime)
  ENDFOREACH(pvexe)
ENDIF(PV_NEED_SHARED_FORWARD)

IF(NOT Q_WS_MAC)
  INSTALL(TARGETS ${PV_EXE_NAME}
    DESTINATION ${PV_EXE_INSTALL_CM24}
    COMPONENT Runtime)
ENDIF(NOT Q_WS_MAC)

# Install system libraries on Linux (will not work on any other Unix
# yet)
FOREACH(fmpglib avcodec avformat avutil)
  IF(FFMPEG_${fmpglib}_LIBRARY)
    GET_FILENAME_COMPONENT(FFMPEG_LIB_DIR_tmp ${FFMPEG_${fmpglib}_LIBRARY} PATH)
    GET_FILENAME_COMPONENT(FFMPEG_LIB_NAME_tmp ${FFMPEG_${fmpglib}_LIBRARY} NAME)
    FILE(GLOB FFMPEG_LIB_LIST RELATIVE ${FFMPEG_LIB_DIR_tmp} "${FFMPEG_${fmpglib}_LIBRARY}*")
    INSTALL(CODE "
MESSAGE(STATUS \"Installing \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR_CM24}/${FFMPEG_LIB_NAME_tmp}\")
EXECUTE_PROCESS (WORKING_DIRECTORY ${FFMPEG_LIB_DIR_tmp}
                 COMMAND tar c ${FFMPEG_LIB_LIST}
                 COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR_CM24})
        " COMPONENT Runtime)
    #INSTALL(FILES ${FFMPEG_${fmpglib}_LIBRARY} DESTINATION ${PV_INSTALL_LIB_DIR_CM24})
    
  ENDIF(FFMPEG_${fmpglib}_LIBRARY)
ENDFOREACH(fmpglib)

SET (QTLIBLIST QTCORE QTGUI QTNETWORK QTXML QTTEST QTSQL)
# the variable and library names are not the same
IF (WIN32)
  SET (QTLIBLIST ${QTLIBLIST} QTASSISTANTCLIENT)
ELSE (WIN32)
  SET (QTLIBLIST ${QTLIBLIST} QTASSISTANT)
ENDIF (WIN32)

IF(NOT Q_WS_MAC)
  FOREACH(qtlib ${QTLIBLIST})
    IF (NOT WIN32)
      #INSTALL(FILES ${QT_${qtlib}_LIBRARY_RELEASE} DESTINATION ${PV_INSTALL_LIB_DIR_CM24})
      GET_FILENAME_COMPONENT(QT_LIB_DIR_tmp ${QT_${qtlib}_LIBRARY_RELEASE} PATH)
      GET_FILENAME_COMPONENT(QT_LIB_NAME_tmp ${QT_${qtlib}_LIBRARY_RELEASE} NAME)
      FILE(GLOB QT_LIB_LIST RELATIVE ${QT_LIB_DIR_tmp} "${QT_${qtlib}_LIBRARY_RELEASE}*")
      INSTALL(CODE "
MESSAGE(STATUS \"Installing \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR_CM24}/${QT_LIB_NAME_tmp}\")
EXECUTE_PROCESS (WORKING_DIRECTORY ${QT_LIB_DIR_tmp}
                 COMMAND tar c ${QT_LIB_LIST}
                 COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR_CM24})
        " COMPONENT Runtime)
    ELSE (NOT WIN32)
      GET_FILENAME_COMPONENT(QT_DLL_PATH_tmp ${QT_QMAKE_EXECUTABLE} PATH)
      INSTALL(FILES ${QT_DLL_PATH_tmp}/${qtlib}4.dll DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime)
    ENDIF (NOT WIN32)
    
  ENDFOREACH(qtlib)
ENDIF(NOT Q_WS_MAC)

# Hard-coded install rules for anything left to install. These should
# probably go to a local cmake file. Change to match the build system's
# libraries
# INSTALL(FILES /usr/lib/libstdc++.so.5 /lib/libgcc_s.so.1 /usr/lib/libpng.so.3 /usr/lib/libexpat.so.0 /usr/lib/libfontconfig.so.1 /usr/lib/libfreetype.so.6 /lib/libz.so.1 DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime)

# We also need Qt assistant
IF(WIN32)
  INSTALL(FILES ${QT_BINARY_DIR}/assistant.exe
    DESTINATION ${PV_EXE_INSTALL_CM24} 
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
    COMPONENT Runtime)
ELSE(WIN32)
  IF (NOT Q_WS_MAC)
    IF(PV_NEED_SHARED_FORWARD)
      INSTALL(FILES ${QT_BINARY_DIR}/assistant 
        DESTINATION ${PV_EXE_INSTALL_CM24} 
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
        COMPONENT Runtime
        RENAME assistant-real)
    ELSE(PV_NEED_SHARED_FORWARD)
      INSTALL(FILES ${QT_BINARY_DIR}/assistant 
        DESTINATION ${PV_EXE_INSTALL_CM24} 
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
        COMPONENT Runtime)
    ENDIF(PV_NEED_SHARED_FORWARD)
  ENDIF (NOT Q_WS_MAC)
ENDIF(WIN32)

# Call this one last so that it happens after the above targets are all installed:
#
IF(Q_WS_MAC)
  INSTALL(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/CreateBundleDuringMakeInstall.cmake")
ENDIF(Q_WS_MAC)
