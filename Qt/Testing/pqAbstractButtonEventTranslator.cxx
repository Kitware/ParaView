/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractButtonEventTranslator.h"

#include <QAbstractButton>
#include <QEvent>

pqAbstractButtonEventTranslator::pqAbstractButtonEventTranslator() :
  CurrentObject(0)
{
}

bool pqAbstractButtonEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& Error)
{
  QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      this->CurrentObject = Object;
      if(object->isCheckable())
        connect(object, SIGNAL(clicked(bool)), this, SLOT(onToggled(bool)));
      else
        connect(object, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
      break;
    case QEvent::Leave:
      disconnect(Object, 0, this, 0);
      this->CurrentObject = 0;
      break;
    default:
      break;
    }
      
  return true;
}

void pqAbstractButtonEventTranslator::onClicked(bool)
{
  emit recordEvent(this->CurrentObject, "activate", "");
}

void pqAbstractButtonEventTranslator::onToggled(bool Checked)
{
  emit recordEvent(this->CurrentObject, "set_boolean", Checked ? "true" : "false");
}

