/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqSpinBoxEventTranslator.h"

#include <QSpinBox>
#include <QEvent>

pqSpinBoxEventTranslator::pqSpinBoxEventTranslator() :
  CurrentObject(0)
{
}

bool pqSpinBoxEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& Error)
{
  QSpinBox* const object = qobject_cast<QSpinBox*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      this->CurrentObject = Object;
      connect(object, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
      break;
    case QEvent::Leave:
      disconnect(Object, 0, this, 0);
      this->CurrentObject = 0;
      break;
    }
      
  return true;
}

void pqSpinBoxEventTranslator::onValueChanged(int Value)
{
  emit recordEvent(this->CurrentObject, "set_int", QString().setNum(Value));
}
