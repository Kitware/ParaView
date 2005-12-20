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
  CurrentObject(0)
{
}

bool pqComboBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QComboBox* const object = qobject_cast<QComboBox*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      if(this->CurrentObject != Object)
        {
        if(this->CurrentObject)
          {
          disconnect(this->CurrentObject, 0, this, 0);
          }
        
        this->CurrentObject = Object;
        connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(onDestroyed(QObject*)));
        connect(object, SIGNAL(activated(const QString&)), this, SLOT(onStateChanged(const QString&)));
        connect(object, SIGNAL(editTextChanged(const QString&)), this, SLOT(onStateChanged(const QString&)));
        }
      break;
      
    default:
      break;
    }

  return true;
}

void pqComboBoxEventTranslator::onDestroyed(QObject* Object)
{
  this->CurrentObject = 0;
}

void pqComboBoxEventTranslator::onStateChanged(const QString& State)
{
  emit recordEvent(this->CurrentObject, "set_string", State);
}
