/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqMenuEventTranslator.h"
#include "pqMenuEventTranslatorAdaptor.h"

#include <QEvent>
#include <QMenu>

pqMenuEventTranslator::pqMenuEventTranslator()
{
}

pqMenuEventTranslator::~pqMenuEventTranslator()
{
  clearActions();
}

bool pqMenuEventTranslator::translateEvent(QObject* Object, QEvent* Event)
{
  QMenu* const object = qobject_cast<QMenu*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      {
      QList<QAction*> actions = object->actions();
      for(int i = 0; i != actions.size(); ++i)
        {
        pqMenuEventTranslatorAdaptor* const adaptor = new pqMenuEventTranslatorAdaptor(actions[i]);
        this->actions.push_back(adaptor);
        QObject::connect(
          adaptor,
          SIGNAL(recordEvent(QObject*, const QString&, const QString&)),
          this,
          SLOT(onRecordEvent(QObject*, const QString&, const QString&)));
        }
      }
      break;
    case QEvent::Leave:
      this->clearActions();
      break;
    }
      
  return true;
}

void pqMenuEventTranslator::clearActions()
{
  for(int i = 0; i != this->actions.size(); ++i)
    delete this->actions[i];
    
  this->actions.clear();
}

void pqMenuEventTranslator::onRecordEvent(QObject* Object, const QString& Command, const QString& Arguments)
{
  emit recordEvent(Object, Command, Arguments);
}
