/*=========================================================================

   Program:   ParaQ
   Module:    pqEventPlayer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqAbstractActivateEventPlayer.h"
#include "pqAbstractBooleanEventPlayer.h"
#include "pqAbstractDoubleEventPlayer.h"
#include "pqAbstractIntEventPlayer.h"
#include "pqAbstractItemViewEventPlayer.h"
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
  addWidgetEventPlayer(new pqAbstractItemViewEventPlayer());
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

