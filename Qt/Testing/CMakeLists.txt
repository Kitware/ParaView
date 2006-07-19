PROJECT(QtTesting)

INCLUDE_DIRECTORIES(
  ${QtTesting_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqAbstractButtonEventTranslator.h
  pqAbstractItemViewEventTranslator.h
  pqAbstractSliderEventTranslator.h
  pqBasicWidgetEventTranslator.h
  pqComboBoxEventTranslator.h
  pqDoubleSpinBoxEventTranslator.h
  pqEventObserverStdout.h
  pqEventObserverXML.h
  pqEventPlayer.h
  pqEventPlayerXML.h
  pqEventTranslator.h
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.h
  pqRecordEventsDialog.h
  pqSpinBoxEventTranslator.h
  pqWidgetEventTranslator.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

QT4_WRAP_UI(UI_BUILT_SOURCES
  pqRecordEventsDialog.ui
)

ADD_LIBRARY(QtTesting
  QtTestingExport.h
  pqAbstractActivateEventPlayer.cxx
  pqAbstractActivateEventPlayer.h
  pqAbstractBooleanEventPlayer.cxx
  pqAbstractBooleanEventPlayer.h
  pqAbstractButtonEventTranslator.cxx
  pqAbstractButtonEventTranslator.h
  pqAbstractDoubleEventPlayer.cxx
  pqAbstractDoubleEventPlayer.h
  pqAbstractIntEventPlayer.cxx
  pqAbstractIntEventPlayer.h
  pqAbstractItemViewEventPlayer.cxx
  pqAbstractItemViewEventPlayer.h
  pqAbstractItemViewEventTranslator.cxx
  pqAbstractItemViewEventTranslator.h
  pqAbstractSliderEventTranslator.cxx
  pqAbstractSliderEventTranslator.h
  pqAbstractStringEventPlayer.cxx
  pqAbstractStringEventPlayer.h
  pqBasicWidgetEventPlayer.cxx
  pqBasicWidgetEventPlayer.h
  pqBasicWidgetEventTranslator.cxx
  pqBasicWidgetEventTranslator.h
  pqComboBoxEventTranslator.cxx
  pqComboBoxEventTranslator.h
  pqDoubleSpinBoxEventTranslator.cxx
  pqDoubleSpinBoxEventTranslator.h
  pqEventObserverStdout.cxx
  pqEventObserverStdout.h
  pqEventObserverXML.cxx
  pqEventObserverXML.h
  pqEventPlayer.cxx
  pqEventPlayer.h
  pqEventPlayerXML.cxx
  pqEventPlayerXML.h
  pqEventTranslator.cxx
  pqEventTranslator.h
  pqLineEditEventTranslator.cxx
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.cxx
  pqMenuEventTranslator.h
  pqObjectNaming.cxx
  pqObjectNaming.h
  pqRecordEventsDialog.cxx
  pqRecordEventsDialog.h
  pqSpinBoxEventTranslator.cxx
  pqSpinBoxEventTranslator.h
  pqTesting.cxx
  pqTesting.h
  pqWidgetEventPlayer.h
  pqWidgetEventTranslator.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtTesting
  ${QT_LIBRARIES}
  ${QT_XML_LIBRARIES}
)

INSTALL(TARGETS QtTesting
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
