PROJECT (PQGUI)

#########################################################################
# Setup Qt
SET(QT_REQUIRED TRUE)
FIND_PACKAGE(Qt4)
SET(QT_USE_QTDESIGNER ON)
SET(QT_USE_QTNETWORK TRUE)
SET(QT_USE_QTXML TRUE)
SET(QT_USE_QTTEST TRUE)
# This ensures that the Qt includes dir etc are set properly.
# We don't need to find the Qt4 package or include the use file
# in any of the SUBDIRS in this directory.
INCLUDE(${QT_USE_FILE})

#########################################################################
# Include Qt dependent code.
SUBDIRS(Testing Widgets Chart)

IF(PARAVIEW_EMBED_PYTHON)
  SUBDIRS(Python)
  SET (MOC_INCLUDE_DIRS
    ${MOC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Python
    )
ENDIF(PARAVIEW_EMBED_PYTHON)

# Include Qt, VTK & Server Manager dependent code.
SUBDIRS(Core Components)

