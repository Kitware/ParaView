PROJECT(pqClient)

SET(QT_USE_QTASSISTANT TRUE)
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
SET(PV_EXE_LIST paraview_alpha)
# Add Qt assistant to the list of shared lib. forwarded executables.
# If this is enabled, the build system will create an executable
# call assistant. This executable will set the appropriate ld library
# path and execute assistant-real. It is the installation's requirement
# to install assistant-real
#SET(PV_EXE_LIST ${PV_EXE_LIST} assistant)

ADD_EXECUTABLE(paraview_alpha${PV_EXE_SUFFIX} WIN32 MACOSX_BUNDLE
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

TARGET_LINK_LIBRARIES(paraview_alpha${PV_EXE_SUFFIX}
  pqComponents
  pqCore
  QtWidgets
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
#    TARGET paraview_alpha${PV_EXE_SUFFIX}
#    POST_BUILD
#    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CompleteBundle.cmake")
ENDIF(APPLE)

IF(WIN32)
  TARGET_LINK_LIBRARIES(paraview_alpha${PV_EXE_SUFFIX}
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)

SET(PV_CLIENT_DOC "${CMAKE_CURRENT_SOURCE_DIR}/pqClient.adp")
STRING(REGEX REPLACE "[^/]+$" "" PV_ASSISTANT "${QT_QMAKE_EXECUTABLE}")
IF(WIN32)
  STRING(REGEX REPLACE "/" "\\\\" PV_ASSISTANT "${PV_ASSISTANT}")
  STRING(REGEX REPLACE "/" "\\\\" PV_CLIENT_DOC "${PV_CLIENT_DOC}")
ENDIF(WIN32)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqClientDocFinder.txt.in"
               "${EXECUTABLE_OUTPUT_PATH}/pqClientDocFinder.txt"
               @ONLY IMMEDIATE)
  
IF(BUILD_TESTING)
  IF(APPLE)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview_alpha.app/Contents/MacOS/paraview_alpha --test-directory=${PARAVIEW_TEST_DIR})
  ELSE(APPLE)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview_alpha --test-directory=${PARAVIEW_TEST_DIR})
  ENDIF(APPLE)

  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_PYTHON ${pqClient_SOURCE_DIR}/Testing/Python)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  SET (XML_TESTS_WITH_BASELINES
    CreateSourceWithoutConnect
    ImageShrink
    LoadStateSelection
    OpenFile
    OpenSaveData
    SimpleInteraction
    SelectionUndo
    VariableSelector
    ViewSettingsDialog
    )

  SET (TESTS_WITHOUT_BASELINES
    AboutDialog
    LoadStateHistogram
    )

  IF(QT_TESTING_WITH_PYTHON)
    SET(PYTHON_TESTS
      #HistogramChart
      ExodusXML
      LoadStateMultiView
      OpenHelp
      VCRAnimation
      )
  ENDIF(QT_TESTING_WITH_PYTHON)

  FOREACH(test ${TESTS_WITHOUT_BASELINES})
    ADD_TEST(pqClient${test}
      ${TEST_BINARY} -dr --run-test=${TEST_XML}/${test}.xml --exit)
  ENDFOREACH(test)
   
  FOREACH(test ${XML_TESTS_WITH_BASELINES})
    ADD_TEST(pqClient${test}
      ${TEST_BINARY} -dr --run-test=${TEST_XML}/${test}.xml
      --compare-view=${TEST_IMAGE}/${test}.png
      --exit)
  ENDFOREACH(test) 
  
  FOREACH(test ${PYTHON_TESTS})
    ADD_TEST(pqClient${test}-Python
      ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
      --exit)
  ENDFOREACH(test) 

# Now we add tests that use the pvTestDriver for running.
  SET(XML_TESTS_WITH_BASELINES
    3DWidgetInCustomFilter
    Cut
    Calculator
    Clip
    ClipHidden
    ClipUndo
    Contour
    ContourRange
    CreateCustomFilter
    CreateSource
    CustomFilter
    CutMulti
    ExodusPanel
    LoadState
    ObjectInspectorSphere
    Plot3DReader
    RepresentationSelector
    SelectReader
    StreamTracer
    StreamTracerUpdates
    Threshold
    UndoRedo
  )

  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver${PV_EXE_SUFFIX})

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

    ADD_TEST(pqClient${test}-ClientRenderServer
      ${TEST_BINARY} --test-render-server
      --test-directory=${PARAVIEW_TEST_DIR}
      -dr
      --run-test=${TEST_XML}/${test}.xml
      --compare-view=${TEST_IMAGE}/${test}.png
      --exit)
  ENDFOREACH(test)

  SUBDIRS(Testing/Cxx)
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

INSTALL(TARGETS paraview_alpha${PV_EXE_SUFFIX}
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

FOREACH(qtlib QTCORE QTGUI QTDESIGNER QTNETWORK QTXML QTTEST)
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

SET(dir "${ParaView_SOURCE_DIR}/Applications/Client/doc/")
FILE(GLOB list_of_files "${dir}/*")
FOREACH(file ${list_of_files})
  IF (${file} MATCHES CVS)
  ELSE (${file} MATCHES CVS)
    INSTALL(FILES ${file}
      DESTINATION ${PV_EXE_INSTALL_CM24}/doc COMPONENT Runtime )
  ENDIF (${file} MATCHES CVS)
ENDFOREACH(file)

INSTALL(FILES ${ParaView_SOURCE_DIR}/Applications/Client/pqClient.adp 
  DESTINATION ${PV_EXE_INSTALL_CM24} COMPONENT Runtime )

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

