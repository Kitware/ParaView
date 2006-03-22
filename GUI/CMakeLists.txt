PROJECT (PQGUI)

#########################################################################
# Setup Qt
FIND_PACKAGE(Qt4)
SET(QT_USE_QTDESIGNER ON)
SET(QT_USE_QTXML TRUE)
# This ensures that the Qt includes dir etc are set properly.
# We don't need to find the Qt4 package or include the use file
# in any of the SUBDIRS in this directory.
INCLUDE(${QT_USE_FILE})

ADD_DEFINITIONS(-DVERSION=${PARAQ_VERSION})

#########################################################################
# Include Qt dependent code.
SUBDIRS(Widgets)

# Include Qt and VTK dependent code.
SUBDIRS(Client)

