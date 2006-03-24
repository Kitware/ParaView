PROJECT (Widgets)

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
)

SUBDIRS(
  Chart
  Testing
)

## Add the source (*.cxx,*.h) files here.
#SET(Widgets_SRCS
#
#  )
#
##########################################################################
#INCLUDE_DIRECTORIES(
#  ${Widgets_SOURCE_DIR}
#  ${Widgets_BINARY_DIR}
#)
#
##########################################################################
## Add the header files that need to moc processed.
#QT4_WRAP_CPP(MOC_BUILT_SOURCES
#
#)
#
##########################################################################
## Add *.ui files to be uic processed.
#SET(UI_FORMS
#)
#
#QT4_WRAP_UI(UI_BUILT_SOURCES
#  ${UI_FORMS}
#)
#
##########################################################################
## Add resources.
#SET(UI_RESOURCES
#)
#
#QT4_ADD_RESOURCES(RCS_SOURCES
#  ${UI_RESOURCES}
#)
#
#
#SOURCE_GROUP("Resources" FILES
#  ${UI_RESOURCES}
#  ${UI_FORMS}
#)
#
#SOURCE_GROUP("Generated" FILES
#  ${MOC_BUILT_SOURCES}
#  ${RCS_SOURCES}
#  ${UI_BUILT_SOURCES}
#)
#
#
##########################################################################
#ADD_LIBRARY(Widgets
#  ${Widgets_SRCS}  
#  ${MOC_BUILT_SOURCES}
#  ${RCS_SOURCES}
#  ${UI_BUILT_SOURCES}
#)
#
#
## Widgets does not depend on VTK/ParaView. Only Qt.
#TARGET_LINK_LIBRARIES(Widgets
#  ${QT_LIBRARIES})
