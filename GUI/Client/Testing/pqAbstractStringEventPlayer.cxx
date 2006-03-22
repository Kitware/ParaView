/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractStringEventPlayer.h"

#include <QComboBox>
#include <QLineEdit>
#include <QtDebug>

pqAbstractStringEventPlayer::pqAbstractStringEventPlayer()
{
}

bool pqAbstractStringEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  if(Command != "set_string")
    return false;

  const QString value = Arguments;
    
  if(QComboBox* const object = qobject_cast<QComboBox*>(Object))
    {
    object->setEditText(value);
    return true;
    }

  if(QLineEdit* const object = qobject_cast<QLineEdit*>(Object))
    {
    object->setText(value);
    return true;
    }

  qCritical() << "calling set_string on unhandled type " << Object;

  Error = true;
  return true;
}

