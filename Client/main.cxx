// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqEventObserverStdout.h"
#include "pqEventTranslator.h"
#include "pqMainWindow.h"

#ifdef PARAQ_BUILD_TESTING
#  include "pqTesting.h"
#endif

#include <QApplication>

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  QApplication qapplication(argc, argv);
  pqMainWindow qwindow(qapplication);
  qwindow.resize(400, 400);
  qwindow.show();
  
  pqEventTranslator event_translator;
  pqEventObserverStdout event_observer;
  QObject::connect(
    &event_translator,
    SIGNAL(abstractEvent(const QString&, const QString&, const QString&)),
    &event_observer,
    SLOT(onAbstractEvent(const QString&, const QString&, const QString&)));
  
  for(int i = 1; i < argc; ++i)
    {
    const QString argument = argv[i];
    
#ifdef PARAQ_BUILD_TESTING
    if(argument == "--runtests")
      {
      pqRunRegressionTests(&qwindow);
      continue;
      }
#endif

    if(argument == "--exit")
      {
      return 0;
      }
    }
  
  return qapplication.exec();
}

