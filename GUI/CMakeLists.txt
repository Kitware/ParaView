PROJECT(PVGUI)

SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
SET(VTK_TCL_HOME ${PVGUI_SOURCE_DIR})
INCLUDE_DIRECTORIES(
  ${TCL_INCLUDE_PATH} ${TK_INCLUDE_PATH}
  ${KWCommon_SOURCE_DIR}
  ${VTK_SOURCE_DIR}/Utilities/zlib
  ${VTK_BINARY_DIR}/Utilities/zlib
  )

IF (WIN32)

 FIND_PATH( HTML_HELP_PATH htmlhelp.h
           "c:/program files/html help workshop"
           "c:/program files/html help workshop/include"
           "[HKEY_CURRENT_USER\\Software\\Microsoft\\HTML Help Workshop;InstallDir]/include" )

 IF (HTML_HELP_PATH)
   INCLUDE_DIRECTORIES(${HTML_HELP_PATH})
 ELSE (HTML_HELP_PATH)
   SET (PARAVIEW_CAN_BUILD 0)
   MESSAGE("ParaView requires htmlhelp to build. Please install html help."
           "You can find html help on the msdn web site here:\n"
           "http://msdn.microsoft.com/\n"
           "http://msdn.microsoft.com/library/default.asp?url=/library/en-us/htmlhelp/html/vsconwhtshw.asp")
 ENDIF (HTML_HELP_PATH)


 FIND_LIBRARY( HTML_HELP_LIBRARY htmlhelp
              "c:/program files/html help workshop"
              "c:/program files/html help workshop/LIB"
              "[HKEY_CURRENT_USER\\Software\\Microsoft\\HTML Help Workshop;InstallDir]/lib")

 IF (HTML_HELP_LIBRARY)
   LINK_LIBRARIES( ${HTML_HELP_LIBRARY} )
 ELSE (HTML_HELP_LIBRARY)
   SET (PARAVIEW_CAN_BUILD 0)
   MESSAGE("ParaView requires htmlhelp to build. Please set HTML_HELP_PATH")
 ENDIF (HTML_HELP_LIBRARY)
 MARK_AS_ADVANCED(HTML_HELP_LIBRARY HTML_HELP_PATH)
ENDIF (WIN32)

SUBDIRS(Widgets)

SUBDIRS(Client)

IF(BUILD_TESTING)
  SET (PARAVIEW_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/paraview)
  SET(PARAVIEW_CS_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/TestClientServer)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
