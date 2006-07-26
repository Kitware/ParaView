/*=========================================================================

   Program: ParaView
   Module:    pqEventPlayer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqBasicWidgetEventPlayer.h"
#include "pqEventPlayer.h"
#include "pqObjectNaming.h"
#include "pqTesting.h"

#include <QApplication>
#include <QObject>
#include <QWidget>
#include <QtDebug>
#include <QAbstractEventDispatcher>

pqEventPlayer::pqEventPlayer()
{
  // connect our play event queue
  QObject::connect(this, 
                   SIGNAL(signalPlayEvent(const QString&, 
                                          const QString&, 
                                          const QString&)),
                   this,
                   SLOT(internalPlayEvent(const QString&, 
                                          const QString&, 
                                          const QString&)),
                   Qt::QueuedConnection);
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
  addWidgetEventPlayer(new pqBasicWidgetEventPlayer());
}

void pqEventPlayer::addWidgetEventPlayer(pqWidgetEventPlayer* Player)
{
  if(Player)
    {
    this->Players.push_back(Player);
    }
}

void pqEventPlayer::playEvent(const QString& Object,
       const QString& Command, const QString& Arguments)
{
  // queue this on the event loop
  // so internalPlayEvent will actually do it

  // queuing it prevents callers from being blocked
  emit this->signalPlayEvent(Object, Command, Arguments);
}

void pqEventPlayer::internalPlayEvent(const QString& Object, 
        const QString& Command, const QString& Arguments)
{
  // if naming doesn't work out, let quit
  QObject* const object = pqObjectNaming::GetObject(Object);
  if(!object)
    {
    this->exit(false);
    return;
    }

  // loop through players and have one of them handle our event
  for(int i = 0; i != this->Players.size(); ++i)
    {
    bool error = false;
    if(this->Players[i]->playEvent(object, Command, Arguments, error))
      {
      if(error)
        {
        qCritical() << "Error playing command " << Command << " object " << object;
        this->exit(false);
        return;
        }
      return;
      }
    }

  qCritical() << "No player for command " << Command << " object " << object;
  this->exit(false);
  return;
}

bool pqEventPlayer::exec()
{
  // aboutToBlock() from QAbstractEventDispatcher
  // is an indication that *absolutely* every Qt event 
  // has been processed, that's the time when we can say
  // "let's play the next test event"
  QObject::connect(QAbstractEventDispatcher::instance(),
                   SIGNAL(aboutToBlock()),
                   this, SIGNAL(readyPlayEvent()));

  this->TopLevelWidgets.clear();
  QWidgetList widgets = QApplication::topLevelWidgets();
  foreach(QWidget* w, widgets)
    {
    this->TopLevelWidgets.append(w);
    }

  // start the loop
  int ret = EventLoop.exec();
  
  QObject::disconnect(QAbstractEventDispatcher::instance(),
                   SIGNAL(aboutToBlock()),
                   this, SIGNAL(readyPlayEvent()));

  return ret == 0 ? true : false;
}

void pqEventPlayer::exit(bool ret)
{
  // any visible top level widgets made while playing tests 
  // will be closed
  // some/all may be modal, and to exit properly, we have
  // to get rid of the modal ones
  foreach(QWidget* w, QApplication::topLevelWidgets())
    {
    if(!this->TopLevelWidgets.contains(w) && w->isVisible())
      {
      w->hide();
      w->close();
      }
    }

  // exit our event loop
  this->EventLoop.exit(ret ? 0 : 1);
}


