PROJECT (PQGUI)

#########################################################################
# Setup Qt
FIND_PACKAGE(Qt4)

IF(QT4_FOUND)

# To make support of ParaView tractable, we only support a single version
# of Qt at one time.  If you really want to use a newer version of Qt, you
# can delete this check, but you are on your own.
# STRING(REGEX MATCH "^4\\.2\\.[0-9]+" qt_version_tmp "${QTVERSION}")
# IF (NOT qt_version_tmp)
#  MESSAGE(SEND_ERROR "Qt ${QTVERSION} not supported.  Please use Qt 4.2.")
# ENDIF (NOT qt_version_tmp)

SET (QT_USE_QTHELP TRUE)
SET (QT_USE_QTUITOOLS TRUE)
SET (QT_USE_QTXML TRUE)  # uitools depends on xml
SET (QT_USE_QTASSISTANTCLIENT TRUE)
# This ensures that the Qt includes dir etc are set properly.
# We don't need to find the Qt4 package or include the use file
# in any of the SUBDIRS in this directory.
INCLUDE(${QT_USE_FILE})


#########################################################################
# Include Qt dependent code.
ADD_SUBDIRECTORY(Testing)
ADD_SUBDIRECTORY(Widgets)

# Include Qt, VTK & Server Manager dependent code.
ADD_SUBDIRECTORY(Core)

IF(PARAVIEW_ENABLE_PYTHON)
  ADD_SUBDIRECTORY(Python)
  SET (MOC_INCLUDE_DIRS
    ${MOC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Python
    )
ENDIF(PARAVIEW_ENABLE_PYTHON)

ADD_SUBDIRECTORY(Components)
ADD_SUBDIRECTORY(ApplicationComponents)

ENDIF(QT4_FOUND)
