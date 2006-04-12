PROJECT(QtTesting)

INCLUDE_DIRECTORIES(
  ${ParaQ_SOURCE_DIR}/VTK/Imaging
  ${QtTesting_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqAbstractButtonEventTranslator.h
  pqAbstractItemViewEventTranslator.h
  pqAbstractSliderEventTranslator.h
  pqComboBoxEventTranslator.h
  pqDoubleSpinBoxEventTranslator.h
  pqEventObserverStdout.h
  pqEventObserverXML.h
  pqEventTranslator.h
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.h
  pqMenuEventTranslatorAdaptor.h
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
  pqImageComparison.cxx
  pqImageComparison.h
  pqLineEditEventTranslator.cxx
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.cxx
  pqMenuEventTranslator.h
  pqMenuEventTranslatorAdaptor.cxx
  pqMenuEventTranslatorAdaptor.h
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
  vtkIO
  vtkRendering
  ${QT_LIBRARIES}
  ${QT_XML_LIBRARIES}
)

