/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractDoubleEventPlayer.h"

#include <QDoubleSpinBox>
#include <QtDebug>

pqAbstractDoubleEventPlayer::pqAbstractDoubleEventPlayer()
{
}

bool pqAbstractDoubleEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  if(Command != "set_double")
    return false;

  const double value = Arguments.toDouble();
    
  if(QDoubleSpinBox* const object = qobject_cast<QDoubleSpinBox*>(Object))
    {
    object->setValue(value);
    return true;
    }

  qCritical() << "calling set_double on unhandled type " << Object;
  Error = true;
  return true;
}

