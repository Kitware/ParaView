
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(QtTesting)

FIND_PACKAGE(Qt4)
SET(QT_USE_QTXML TRUE)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${QtTesting_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Imaging
)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqAbstractButtonEventTranslator.h
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

QT4_WRAP_UI(UI_BUILT_SOURCES
  pqRecordEventsDialog.ui
)

ADD_LIBRARY(QtTesting
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
  pqRecordEventsDialog.cxx
  pqRecordEventsDialog.h
  pqSpinBoxEventTranslator.cxx
  pqSpinBoxEventTranslator.h
  pqWidgetEventPlayer.h
  pqWidgetEventTranslator.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtTesting
  vtkIO
  vtkRendering
  ${QT_LIBRARIES}
  ${QT_XML_LIBRARIES}
)

