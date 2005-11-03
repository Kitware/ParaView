/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCheckBoxEventTranslator.h"

#include <QCheckBox>
#include <QEvent>

pqCheckBoxEventTranslator::pqCheckBoxEventTranslator() :
  currentObject(0)
{
}

bool pqCheckBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event)
{
  QCheckBox* const object = qobject_cast<QCheckBox*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      this->currentObject = Object;
      connect(object, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
      break;
    case QEvent::Leave:
      disconnect(Object, 0, this, 0);
      this->currentObject = 0;
      break;
    }
      
  return true;
}

void pqCheckBoxEventTranslator::onStateChanged(int State)
{
  emit recordEvent(this->currentObject, "set_boolean", State ? "true" : "false");
}
