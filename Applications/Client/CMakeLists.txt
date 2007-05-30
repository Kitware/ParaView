PROJECT(pqClient)

SET(QT_USE_QTASSISTANT TRUE)
SET(QT_USE_QTUITOOLS TRUE)
SET(QT_USE_QTNETWORK TRUE)
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

IF(APPLE)
  SET(apple_bundle_sources "MacIcon.icns")
  SET_SOURCE_FILES_PROPERTIES(
    "MacIcon.icns"
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
    )
  SET(MACOSX_BUNDLE_ICON_FILE MacIcon.icns)
ENDIF(APPLE)

# Add main server executables.
SET(PV_EXE_LIST paraview)
# Add Qt assistant to the list of shared lib. forwarded executables.
# If this is enabled, the build system will create an executable
# call assistant. This executable will set the appropriate ld library
# path and execute assistant-real. It is the installation's requirement
# to install assistant-real
#SET(PV_EXE_LIST ${PV_EXE_LIST} assistant)

SET(MACOSX_BUNDLE_BUNDLE_NAME "ParaView")
ADD_EXECUTABLE(paraview${PV_EXE_SUFFIX} WIN32 MACOSX_BUNDLE  
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

TARGET_LINK_LIBRARIES(paraview${PV_EXE_SUFFIX}
  pqComponents
  pqCore
  pqWidgets
  QtChart
  QtTesting
  ${QT_LIBRARIES}
)

IF(APPLE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CompleteBundle.cmake"
    @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CreateBundle.sh"
    @ONLY IMMEDIATE)
#  ADD_CUSTOM_COMMAND(
#    TARGET paraview${PV_EXE_SUFFIX}
#    POST_BUILD
#    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CompleteBundle.cmake")
ENDIF(APPLE)

IF(WIN32)
  TARGET_LINK_LIBRARIES(paraview${PV_EXE_SUFFIX}
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)

EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_BINS"
             OUTPUT_VARIABLE PV_ASSISTANT)
FILE(TO_NATIVE_PATH "${PV_ASSISTANT}/" PV_ASSISTANT)
FILE(TO_NATIVE_PATH "${ParaView_BINARY_DIR}/pqClient.adp" PV_CLIENT_DOC)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqClientDocFinder.txt.in"
               "${EXECUTABLE_OUTPUT_PATH}/pqClientDocFinder.txt"
               @ONLY IMMEDIATE)
  
IF(BUILD_TESTING)
  IF(APPLE)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview --test-directory=${PARAVIEW_TEST_DIR})
  ELSE(APPLE)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
  ENDIF(APPLE)

  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_PYTHON ${pqClient_SOURCE_DIR}/Testing/Python)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  SET (XML_TESTS_WITH_BASELINES
    # ColorEditor3 doesn't work in client/server mode
    ColorEditor3
    CreateSourceWithoutConnect
    # flow crashes in SurfaceVectors in an mpi build in client/server mode
    Flow
#    LoadStateSelection
    OpenFile
    OpenSaveData
    PropertyLink
    # PropertyLink doesn't work in client/server mode
    SimpleInteraction
#    SelectionUndo
    VariableSelector
    ViewSettingsDialog
    )

  SET (TESTS_WITHOUT_BASELINES
    AboutDialog
    )
  IF(PARAVIEW_DATA_ROOT)
    SET (TESTS_WITHOUT_BASELINES
      ${TESTS_WITHOUT_BASELINES}
      LoadStateHistogram
      ElementInspector
      )
  ENDIF(PARAVIEW_DATA_ROOT)

  IF(QT_TESTING_WITH_PYTHON)
    SET(XML_TESTS_WITH_BASELINES ${XML_TESTS_WITH_BASELINES}
      PythonProgrammableFilter
      )
    SET(PYTHON_TESTS
      OpenHelp
      ToolBarAndMenus
      )
    IF(PARAVIEW_DATA_ROOT)
      SET(PYTHON_TESTS ${PYTHON_TESTS}
        #HistogramChart
        ExodusXML
        LoadStateMultiView
        #SaveAnimationMultiView   -- need to fix timing issue
# VCRAnimation -- enable after PVD reader supports time.
        )
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
      ADD_TEST(pqClient${test}
        ${TEST_BINARY} -dr --run-test=${TEST_XML}/${test}.xml
        --test-directory=${PARAVIEW_TEST_DIR}
        --compare-view=${TEST_IMAGE}/${test}.png
        --exit)
    ENDFOREACH(test) 
  ENDIF(PARAVIEW_DATA_ROOT)
  
  FOREACH(test ${PYTHON_TESTS})
    ADD_TEST(pqClient${test}-Python
      ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
      --test-directory=${PARAVIEW_TEST_DIR}
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
      IF(APPLE)
        SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview --test-directory=${PARAVIEW_TEST_DIR})
      ELSE(APPLE)
        SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
      ENDIF(APPLE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)
  

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

  ENDIF(BUILD_SHARED_LIBS AND BUILD_EXAMPLES)


# Now we add tests that use the pvTestDriver for running.
  SET(XML_TESTS_WITH_BASELINES
    3DWidgetInCustomFilter
    CameraLink
    ClipPlaneLink
    Cut
    Calculator
    Clip
    ClipHidden
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
    CutMulti
    D3SmallCells
    DualSphereAnimation
    ExodusIIPanel
    ExodusModeShapes
    ExodusPanel
    Fractal2D
    Fractal3D
    LinePlot
    LoadState
    LoadSaveStateAnimation
    ObjectInspectorSphere
    Plot3DReader
    RepresentationSelector
    SelectReader
    StreamTracer
    StreamTracerUpdates
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
#    XdmfRead
#    XdmfReadImageData
#    XdmfReadImageDataCollection
#    XdmfReadRectilinearGrid
#    XdmfReadRectilinearGridCollection
#    XdmfReadStructuredGrid
#    XdmfReadStructuredGridCollection
  )

  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver)

  IF(PARAVIEW_DATA_ROOT)
    OPTION(PARAVIEW_CLIENT_RENDER_SERVER_TESTS
      "Turn on/off client tests that involve separate data and render servers."
      ON)
    MARK_AS_ADVANCED(PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    FOREACH(test ${XML_TESTS_WITH_BASELINES})
      ADD_TEST(pqClient${test}-Builtin
        ${TEST_BINARY}
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        --exit)
      
      ADD_TEST(pqClient${test}-ClientServer
        ${TEST_BINARY} --test-server
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        --exit)

      IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
        ADD_TEST(pqClient${test}-ClientRenderServer
          ${TEST_BINARY} --test-render-server
          --test-directory=${PARAVIEW_TEST_DIR}
          -dr
          --run-test=${TEST_XML}/${test}.xml
          --compare-view=${TEST_IMAGE}/${test}.png
          --exit)
      ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test)
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

INSTALL(TARGETS paraview${PV_EXE_SUFFIX}
  DESTINATION ${PV_EXE_INSTALL_CM24}
  COMPONENT Runtime)

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

SET (QTLIBLIST QTCORE QTGUI QTNETWORK QTXML QTTEST)
# the variable and library names are not the same
IF (WIN32)
  SET (QTLIBLIST ${QTLIBLIST} QTASSISTANTCLIENT)
ELSE (WIN32)
  SET (QTLIBLIST ${QTLIBLIST} QTASSISTANT)
ENDIF (WIN32)

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

# Hard-coded install rules for anything left to install. These should
# probably go to a local cmake file. Change to match the build system's
# libraries
# INSTALL(FILES /usr/lib/libstdc++.so.5 /lib/libgcc_s.so.1 /usr/lib/libpng.so.3 /usr/lib/libexpat.so.0 /usr/lib/libfontconfig.so.1 /usr/lib/libfreetype.so.6 /lib/libz.so.1 DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime)

# Hard-coded install rules for anything left to install. These should
# probably go to a local cmake file. Change to match the build system's
# assistant. Make sure share forwarding for assistant (see up above) is
# enabled if necessary
# IF(WIN32)
#   INSTALL(FILES C:/Qt/4.1.4/bin/assistant.exe
#     DESTINATION ${PV_EXE_INSTALL_CM24} 
#     PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
#     COMPONENT Runtime)
# ELSE(WIN32)
#   IF(PV_NEED_SHARED_FORWARD)
#     INSTALL(FILES /opt/qt-4.1.3/bin/assistant 
#       DESTINATION ${PV_EXE_INSTALL_CM24} 
#       PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
#       COMPONENT Runtime
#       RENAME assistant-real)
#   ELSE(PV_NEED_SHARED_FORWARD)
#     INSTALL(FILES /opt/qt-4.1.3/bin/assistant 
#       DESTINATION ${PV_EXE_INSTALL_CM24} 
#       PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
#       COMPONENT Runtime)
#   ENDIF(PV_NEED_SHARED_FORWARD)
# ENDIF(WIN32)

