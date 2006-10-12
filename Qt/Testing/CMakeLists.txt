PROJECT(QtTesting)

INCLUDE_DIRECTORIES(
  ${QtTesting_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QT_QTTEST_INCLUDE_DIR}
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
  pqEventDispatcher.h
  pqEventPlayer.h
  pqEventTranslator.h
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.h
  pqRecordEventsDialog.h
  pqSpinBoxEventTranslator.h
  pqStdoutEventObserver.h
  pqTabBarEventTranslator.h
  pqWidgetEventTranslator.h
  pqXMLEventObserver.h
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
  pqEventDispatcher.cxx
  pqEventDispatcher.h
  pqEventPlayer.cxx
  pqEventPlayer.h
  pqEventTranslator.cxx
  pqEventTranslator.h
  pqEventSource.h
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
  pqStdoutEventObserver.cxx
  pqStdoutEventObserver.h
  pqTabBarEventPlayer.cxx
  pqTabBarEventPlayer.h
  pqTabBarEventTranslator.cxx
  pqTabBarEventTranslator.h
  pqWidgetEventPlayer.h
  pqWidgetEventTranslator.h
  pqXMLEventObserver.cxx
  pqXMLEventObserver.h
  pqXMLEventSource.cxx
  pqXMLEventSource.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtTesting
  ${QT_LIBRARIES} 
  ${QT_QTTEST_LIBRARY}
)

INSTALL(TARGETS QtTesting
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
