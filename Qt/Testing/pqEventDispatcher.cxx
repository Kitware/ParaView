/*=========================================================================

   Program: ParaView
   Module:    pqEventDispatcher.cxx

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

#include "pqEventPlayer.h"
#include "pqEventSource.h"
#include "pqEventDispatcher.h"

#include <QAbstractEventDispatcher>
#include <QtDebug>
#include <QTimer>
#include <QApplication>

////////////////////////////////////////////////////////////////////////////
// pqEventDispatcher::pqImplementation

class pqEventDispatcher::pqImplementation
{
public:
  pqImplementation() :
    Source(0),
    Player(0)
  {
#if defined(Q_WS_MAC)
    this->Timer.setInterval(1);
    this->Timer.setSingleShot(true);
#endif
  }
  
  pqEventSource* Source;
  pqEventPlayer* Player;
#if defined(Q_WS_MAC)
  QTimer Timer;
#endif
};

////////////////////////////////////////////////////////////////////////////
// pqEventDispatcher

pqEventDispatcher::pqEventDispatcher() :
  Implementation(new pqImplementation())
{
  QObject::connect(this, SIGNAL(readyPlayNextEvent()),
                   this, SLOT(playNextEvent()));
}

pqEventDispatcher::~pqEventDispatcher()
{
  delete this->Implementation;
}

void pqEventDispatcher::playEvents(pqEventSource& source, pqEventPlayer& player)
{
  if(this->Implementation->Source)
    {
    qCritical() << "Event dispatcher is already playing";
    return;
    }

  this->Implementation->Source = &source;
  this->Implementation->Player = &player;
    
  QApplication::setEffectEnabled(Qt::UI_General, false);

#if defined(Q_WS_MAC)
  QObject::connect(
    QAbstractEventDispatcher::instance(),
    SIGNAL(aboutToBlock()),
    &this->Implementation->Timer,
    SLOT(start()));
  
  QObject::connect(
    &this->Implementation->Timer,
    SIGNAL(timeout()),
    this,
    SIGNAL(readyPlayNextEvent()));
#else
  QObject::connect(
    QAbstractEventDispatcher::instance(),
    SIGNAL(aboutToBlock()),
    this,
    SIGNAL(readyPlayNextEvent()),
    Qt::QueuedConnection);
#endif
}
  
void pqEventDispatcher::playNextEvent()
{

  if(!this->Implementation->Source)
    {
    return;
    }

  QString object;
  QString command;
  QString arguments;
  
  // block signals as some event sources may interact with the event loop
  this->blockSignals(true);

  int result = this->Implementation->Source->getNextEvent(
                              object, command, arguments);
  this->blockSignals(false);

  if(result == pqEventSource::DONE)
    {
    this->stopPlayback();
    emit this->succeeded();
    return;
    }
  else if(result == pqEventSource::FAILURE)
    {
    this->stopPlayback();
    emit this->failed();
    return;
    }
    
  bool error = false;
  // block signals as some event sources may interact with the event loop, as
  // well as some players interact with the event loop
  this->blockSignals(true);
  this->Implementation->Player->playEvent(object, command, arguments, error);
  this->blockSignals(false);
  if(error)
    {
    this->stopPlayback();
    emit this->failed();
    return;
    }
}

void pqEventDispatcher::stopPlayback()
{
#if defined(Q_WS_MAC)
  QObject::disconnect(
    QAbstractEventDispatcher::instance(),
    SIGNAL(aboutToBlock()),
    &this->Implementation->Timer,
    SLOT(start()));
  
  QObject::disconnect(
    &this->Implementation->Timer,
    SIGNAL(timeout()),
    this,
    SIGNAL(readyPlayNextEvent()));
#else
  QObject::disconnect(
    QAbstractEventDispatcher::instance(),
    SIGNAL(aboutToBlock()),
    this,
    SIGNAL(readyPlayNextEvent()));
#endif
    
  this->Implementation->Source = 0;
  this->Implementation->Player = 0;
}

