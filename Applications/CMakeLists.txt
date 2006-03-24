#########################################################################
# Setup Qt
SET(QT_REQUIRED TRUE)
FIND_PACKAGE(Qt4)
SET(QT_USE_QTDESIGNER ON)
SET(QT_USE_QTXML TRUE)
# This ensures that the Qt includes dir etc are set properly.
# We don't need to find the Qt4 package or include the use file
# in any of the SUBDIRS in this directory.
INCLUDE(${QT_USE_FILE})

#########################################################################
# Everything under Application uses VTK/ParaView ServerManager.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaQ_SOURCE_DIR}/VTK/GUISupport/Qt
)

SUBDIRS(
  Client
  DobranoViz
)

