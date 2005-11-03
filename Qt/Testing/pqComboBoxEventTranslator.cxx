/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqComboBoxEventTranslator.h"

#include <QComboBox>
#include <QEvent>

pqComboBoxEventTranslator::pqComboBoxEventTranslator() :
  currentObject(0)
{
}

bool pqComboBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event)
{
  QComboBox* const object = qobject_cast<QComboBox*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      if(this->currentObject != Object)
        {
        if(this->currentObject)
          {
          disconnect(this->currentObject, 0, this, 0);
          }
        
        this->currentObject = Object;
        connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(onDestroyed(QObject*)));
        connect(object, SIGNAL(activated(const QString&)), this, SLOT(onStateChanged(const QString&)));
        connect(object, SIGNAL(editTextChanged(const QString&)), this, SLOT(onStateChanged(const QString&)));
        }
      break;
    }

  return true;
}

void pqComboBoxEventTranslator::onDestroyed(QObject* Object)
{
  this->currentObject = 0;
}

void pqComboBoxEventTranslator::onStateChanged(const QString& State)
{
  emit recordEvent(this->currentObject, "set_string", State);
}
