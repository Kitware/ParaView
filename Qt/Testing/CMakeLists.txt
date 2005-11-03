
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(QtTesting)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqAbstractSliderEventTranslator.h
  pqCheckBoxEventTranslator.h
  pqComboBoxEventTranslator.h
  pqDoubleSpinBoxEventTranslator.h
  pqEventObserverStdout.h
  pqEventObserverXML.h
  pqEventTranslator.h
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.h
  pqMenuEventTranslatorAdaptor.h
  pqPushButtonEventTranslator.h
  pqSpinBoxEventTranslator.h
  pqWidgetEventTranslator.h
)

ADD_LIBRARY(QtTesting
  pqAbstractActivateEventPlayer.cxx
  pqAbstractActivateEventPlayer.h
  pqAbstractBooleanEventPlayer.cxx
  pqAbstractBooleanEventPlayer.h
  pqAbstractDoubleEventPlayer.cxx
  pqAbstractDoubleEventPlayer.h
  pqAbstractIntEventPlayer.cxx
  pqAbstractIntEventPlayer.h
  pqAbstractSliderEventTranslator.cxx
  pqAbstractSliderEventTranslator.h
  pqAbstractStringEventPlayer.cxx
  pqAbstractStringEventPlayer.h
  pqCheckBoxEventTranslator.cxx
  pqCheckBoxEventTranslator.h
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
  pqEventTranslator.cxx
  pqEventTranslator.h
  pqLineEditEventTranslator.cxx
  pqLineEditEventTranslator.h
  pqMenuEventTranslator.cxx
  pqMenuEventTranslator.h
  pqMenuEventTranslatorAdaptor.cxx
  pqMenuEventTranslatorAdaptor.h
  pqPushButtonEventTranslator.cxx
  pqPushButtonEventTranslator.h
  pqSpinBoxEventTranslator.cxx
  pqSpinBoxEventTranslator.h
  pqWidgetEventPlayer.h
  pqWidgetEventTranslator.h
  ${MOC_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtTesting
  ${QT_LIBRARIES}
  ${JPEG_LIBRARY}
  ${PNG_LIBRARY}
  ${FREETYPE_LIBRARY}
)

