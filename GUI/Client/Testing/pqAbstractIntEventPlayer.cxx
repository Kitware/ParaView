/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractIntEventPlayer.h"

#include <QAbstractSlider>
#include <QSpinBox>
#include <QtDebug>

pqAbstractIntEventPlayer::pqAbstractIntEventPlayer()
{
}

bool pqAbstractIntEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  if(Command != "set_int")
    return false;

  const int value = Arguments.toInt();
    
  if(QAbstractSlider* const object = qobject_cast<QAbstractSlider*>(Object))
    {
    object->setValue(value);
    return true;
    }

  if(QSpinBox* const object = qobject_cast<QSpinBox*>(Object))
    {
    object->setValue(value);
    return true;
    }

  qCritical() << "calling set_int on unhandled type " << Object;
  Error = true;
  return true;
}

