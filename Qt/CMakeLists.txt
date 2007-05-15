PROJECT (PQGUI)

#########################################################################
# Setup Qt
SET(QT_REQUIRED TRUE)
SET(QT_MIN_VERSION "4.2.2")
FIND_PACKAGE(Qt4)

IF(NOT QT4_FOUND)
  MESSAGE(SEND_ERROR "Qt ${QT_MIN_VERSION} or greater not found.  Please check the QT_QMAKE_EXECUTABLE variable.")
ELSE(NOT QT4_FOUND)

# help CMake fine QtUiTools on Mac (binary install)
IF(APPLE AND QT4_FOUND AND QT_QTCORE_FOUND AND NOT QT_QTUITOOLS_FOUND)
  FIND_LIBRARY(QT_QTUITOOLS_LIBRARY_RELEASE NAMES QtUiTools PATHS ${QT_LIBRARY_DIR})
  _QT4_ADJUST_LIB_VARS(QTUITOOLS)
ENDIF(APPLE AND QT4_FOUND AND QT_QTCORE_FOUND AND NOT QT_QTUITOOLS_FOUND)

SET(QT_USE_QTUITOOLS TRUE)
SET(QT_USE_QTNETWORK TRUE)
SET(QT_USE_QTXML TRUE)
# This ensures that the Qt includes dir etc are set properly.
# We don't need to find the Qt4 package or include the use file
# in any of the SUBDIRS in this directory.
INCLUDE(${QT_USE_FILE})


#########################################################################
# Include Qt dependent code.
ADD_SUBDIRECTORY(Testing)
ADD_SUBDIRECTORY(Widgets)
ADD_SUBDIRECTORY(Chart)

IF(PARAVIEW_ENABLE_PYTHON)
  ADD_SUBDIRECTORY(Python)
  SET (MOC_INCLUDE_DIRS
    ${MOC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Python
    )
ENDIF(PARAVIEW_ENABLE_PYTHON)

# Include Qt, VTK & Server Manager dependent code.
ADD_SUBDIRECTORY(Core)
ADD_SUBDIRECTORY(Components)

ENDIF(NOT QT4_FOUND)

