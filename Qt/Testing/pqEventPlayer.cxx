/*=========================================================================

   Program: ParaView
   Module:    pqEventPlayer.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

#include "pqAbstractActivateEventPlayer.h"
#include "pqAbstractBooleanEventPlayer.h"
#include "pqAbstractDoubleEventPlayer.h"
#include "pqAbstractIntEventPlayer.h"
#include "pqAbstractItemViewEventPlayer.h"
#include "pqAbstractMiscellaneousEventPlayer.h"
#include "pqAbstractStringEventPlayer.h"
#include "pqBasicWidgetEventPlayer.h"
#include "pqObjectNaming.h"
#include "pqTabBarEventPlayer.h"
#include "pqTreeViewEventPlayer.h"

#include <QApplication>
#include <QWidget>
#include <QtDebug>

pqEventPlayer::pqEventPlayer()
{
}

pqEventPlayer::~pqEventPlayer()
{
}

void pqEventPlayer::addDefaultWidgetEventPlayers()
{
  addWidgetEventPlayer(new pqBasicWidgetEventPlayer());
  addWidgetEventPlayer(new pqAbstractActivateEventPlayer());
  addWidgetEventPlayer(new pqAbstractBooleanEventPlayer());
  addWidgetEventPlayer(new pqAbstractDoubleEventPlayer());
  addWidgetEventPlayer(new pqAbstractIntEventPlayer());
  addWidgetEventPlayer(new pqAbstractItemViewEventPlayer());
  addWidgetEventPlayer(new pqAbstractStringEventPlayer());
  addWidgetEventPlayer(new pqTabBarEventPlayer());
  addWidgetEventPlayer(new pqTreeViewEventPlayer());
  addWidgetEventPlayer(new pqAbstractMiscellaneousEventPlayer());
}

void pqEventPlayer::addWidgetEventPlayer(pqWidgetEventPlayer* Player)
{
  if(Player)
    {
    this->Players.push_front(Player);
    Player->setParent(this);
    }
}

void pqEventPlayer::playEvent(
  const QString& Object, 
  const QString& Command,
  const QString& Arguments,
  bool& Error)
{
  // If we can't find an object with the right name, we're done ...
  QObject* const object = pqObjectNaming::GetObject(Object);
  if(!object)
    {
    Error = true;
    return;
    }

  // Loop through players until the event gets handled ...
  bool accepted = false;
  bool error = false;
  for(int i = 0; i != this->Players.size(); ++i)
    {
    accepted = this->Players[i]->playEvent(object, Command, Arguments, error);
    if(accepted)
      {
      break;
      }
    }

  // The event wasn't handled at all ...
  if(!accepted)
    {
    qCritical() << "Unhandled event " << Command << " object " << object;
    Error = true;
    return;
    }

  // The event was handled, but there was a problem ...    
  if(accepted && error)
    {
    qCritical() << "Event error " << Command << " object " << object;
    Error = true;
    return;
    }

  // The event was handled successfully ...
  Error = false;
}

