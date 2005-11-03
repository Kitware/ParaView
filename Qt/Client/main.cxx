// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqMainWindow.h"

#ifdef PARAQ_BUILD_TESTING
#include <Testing/pqEventObserverStdout.h>
#include <Testing/pqEventObserverXML.h>
#include <Testing/pqEventTranslator.h>
#endif

#include <QApplication>

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  QApplication qapplication(argc, argv);
  pqMainWindow qwindow;
  qwindow.resize(800, 600);
  qwindow.show();
  
#ifdef PARAQ_BUILD_TESTING
  pqEventTranslator event_translator;
  event_translator.addDefaultWidgetEventTranslators();
  
/*
  pqEventObserverStdout event_observer_stdout;
  QObject::connect(
    &event_translator,
    SIGNAL(recordEvent(const QString&, const QString&, const QString&)),
    &event_observer_stdout,
    SLOT(onRecordEvent(const QString&, const QString&, const QString&)));
*/
 
  pqEventObserverXML event_observer_xml(cout);
  QObject::connect(
    &event_translator,
    SIGNAL(recordEvent(const QString&, const QString&, const QString&)),
    &event_observer_xml,
    SLOT(onRecordEvent(const QString&, const QString&, const QString&)));
#endif
  
  for(int i = 1; i < argc; ++i)
    {
    const QString argument = argv[i];
    
    if(argument == "--exit")
      {
      return 0;
      }
    }
  
  return qapplication.exec();
}

