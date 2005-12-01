/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractActivateEventPlayer.h"

#include <QAction>
#include <QPushButton>
#include <QtDebug>

pqAbstractActivateEventPlayer::pqAbstractActivateEventPlayer()
{
}

bool pqAbstractActivateEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& /*Arguments*/, bool& Error)
{
  if(Command != "activate")
    return false;

  if(QAction* const object = qobject_cast<QAction*>(Object))
    {
    object->activate(QAction::Trigger);
    return true;
    }

  if(QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object))
    {
    object->click();
    return true;
    }

  qCritical() << "calling activate on unhandled type " << Object;
  Error = true;
  return true;
}

