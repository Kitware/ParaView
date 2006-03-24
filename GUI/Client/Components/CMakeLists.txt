PROJECT(QtComponents)

INCLUDE_DIRECTORIES(
  ${QtComponents_SOURCE_DIR}
  ${QtComponents_BINARY_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqDelimitedTextParser.h
  pqFileDialog.h
  pqFileDialogModel.h
  pqLocalFileDialogModel.h
  pqServerFileDialogModel.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  pqFileDialog.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)


SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

ADD_LIBRARY(QtComponents
  pqConnect.cxx
  pqConnect.h
  pqDelimitedTextParser.cxx
  pqDelimitedTextParser.h
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqLocalFileDialogModel.cxx
  pqLocalFileDialogModel.h
  pqServerFileDialogModel.cxx
  pqServerFileDialogModel.h
  pqSetData.cxx
  pqSetData.h
  pqSetName.cxx
  pqSetName.h
  pqWaitCursor.cxx
  pqWaitCursor.h
  QtComponentsExport.h
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)
  
TARGET_LINK_LIBRARIES(QtComponents
  vtkCommon
  vtkPVServerCommon
  vtkClientServer
  ${QT_LIBRARIES}
)
