PROJECT (PQGUI)

#########################################################################
# Setup Qt
SET(QT_REQUIRED TRUE)
SET(QT_MIN_VERSION "4.2.2")
FIND_PACKAGE(Qt4)
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

