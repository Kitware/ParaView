/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAbstractActivateEventPlayer.h"
#include "pqAbstractBooleanEventPlayer.h"
#include "pqAbstractDoubleEventPlayer.h"
#include "pqAbstractIntEventPlayer.h"
#include "pqAbstractStringEventPlayer.h"

#include "pqEventPlayer.h"

#include <QApplication>
#include <QObject>
#include <QStringList>
#include <QtDebug>

namespace
{

/// Given a slash-delimited "path", lookup a Qt object hierarchically
QObject* pqFindObjectByTree(QObject& Root, const QString& Path)
{
  QObject* object = &Root;
  const QStringList paths = Path.split("/");
  for(int i = 1; i < paths.size(); ++i) // Note - we're ignoring the top-level path, since it already represents the root node
    {
    if(object)
      object = object->findChild<QObject*>(paths[i]);
    }  
  
  return object;
}

} // namespace

pqEventPlayer::pqEventPlayer(QObject& root) :
  RootObject(root)
{
}

pqEventPlayer::~pqEventPlayer()
{
  for(int i = 0; i != this->Players.size(); ++i)
    delete this->Players[i];
}

void pqEventPlayer::addDefaultWidgetEventPlayers()
{
  addWidgetEventPlayer(new pqAbstractActivateEventPlayer());
  addWidgetEventPlayer(new pqAbstractBooleanEventPlayer());
  addWidgetEventPlayer(new pqAbstractDoubleEventPlayer());
  addWidgetEventPlayer(new pqAbstractIntEventPlayer());
  addWidgetEventPlayer(new pqAbstractStringEventPlayer());
}

void pqEventPlayer::addWidgetEventPlayer(pqWidgetEventPlayer* Player)
{
  if(Player)
    {
    this->Players.push_back(Player);
    }
}

bool pqEventPlayer::playEvent(const QString& Object, const QString& Command, const QString& Arguments)
{
  QObject* object = pqFindObjectByTree(RootObject, Object);
  if(!object)
    {
    qCritical() << "could not locate object " << Object;
    return false;
    }

  for(int i = 0; i != this->Players.size(); ++i)
    {
    bool error = false;
    if(this->Players[i]->playEvent(object, Command, Arguments, error))
      {
      if(error)
        {
        qCritical() << "error playing command " << Command << " object " << object;
        return false;
        }
        
      QApplication::instance()->processEvents();
      return true;
      }
    }

  qCritical() << "no player for command " << Command << " object " << object;
  return false;
}

