/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractBooleanEventPlayer.h"

#include <QAbstractButton>

pqAbstractBooleanEventPlayer::pqAbstractBooleanEventPlayer()
{
}

bool pqAbstractBooleanEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments)
{
  if(Command != "set_boolean")
    return false;

  const bool value = Arguments == "true" ? true : false;

  if(QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object))
    {
    if(value != object->isChecked())
      object->setChecked(value);
    return true;
    }

  return false;
}

