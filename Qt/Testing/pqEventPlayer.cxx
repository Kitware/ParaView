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

#include <QCoreApplication>
#include <QObject>

namespace
{

/// Given a Qt object, lookup a child object by name, assuming that all names are unique
QObject* pqFindObjectByName(QObject& Object, const QString& Name)
{
  if(QObject* child = Object.findChild<QObject*>(Name))
    return child;
    
  QObjectList children = Object.children();
  for(int i = 0; i != children.size(); ++i)
    {
    if(QObject* child = pqFindObjectByName(*children[i], Name))
      return child;
    }

  return 0;       
}

} // namespace

pqEventPlayer::pqEventPlayer(QObject& RootObject) :
  root_object(RootObject)
{
}

pqEventPlayer::~pqEventPlayer()
{
  for(int i = 0; i != this->players.size(); ++i)
    delete this->players[i];
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
    this->players.push_back(Player);
    }
}

bool pqEventPlayer::playEvent(const QString& Object, const QString& Command, const QString& Arguments)
{
  QObject* object = pqFindObjectByName(root_object, Object);
  if(!object)
    return false;

  for(int i = 0; i != this->players.size(); ++i)
    {
    if(this->players[i]->playEvent(object, Command, Arguments))
      return true;
    }
    
  return false;
}

