/*=========================================================================

   Program: ParaView
   Module:    pqEventPlayer.h

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

#ifndef _pqEventPlayer_h
#define _pqEventPlayer_h

#include "QtTestingExport.h"

#include <QObject>
#include <QString>
#include <QList>

class pqWidgetEventPlayer;

/**
Manages playback of test-cases, demos, tutorials, etc.
pqEventPlayer converts high-level ParaView events 
(button click, row selection, etc) into low-level Qt events 
that drive the user interface.

The high-level events created by pqEventTranslator are fed to 
pqEventPlayer::playEvent() one-by-one, which passes each event 
through a collection of pqWidgetEventPlayer objects.  Each
pqWidgetEventPlayer has the chance to ignore the event or
accept and update the program state using low-level Qt events.

pqEventDispatcher is an example of an object that can retrieve 
high-level events from a pqEventSource object, feeding them
to pqEventPlayer.

For specialized Qt widgets it may be necessary to create "custom" 
pqWidgetEventPlayer derivatives, which can be added to pqEventPlayer 
using the addWidgetEventPlayer() method.  
pqWidgetEventPlayer objects are searched in order for 
event playback, so you can also use this mechanism to
"override" the builtin event players.

\sa pqWidgetEventPlayer, pqEventTranslator, pqEventDispatcher,
pqEventSource, pqXMLEventSource
*/

class QTTESTING_EXPORT pqEventPlayer :
  public QObject
{
  Q_OBJECT
  
public:
  pqEventPlayer();
  ~pqEventPlayer();

  /** Adds the default set of widget players to the current working set.  
  Players are executed in-order, so you can call addWidgetEventPlayer() 
  before this function to override default players. */
  void addDefaultWidgetEventPlayers();
  /** Adds a new player to the current working set of widget players.  
  pqEventPlayer assumes control of the lifetime of the supplied object. */
  void addWidgetEventPlayer(pqWidgetEventPlayer*);
  
  /** This method is called with each high-level ParaView event, which 
  will invoke the corresponding low-level Qt functionality in-turn.  
  If there was an error playing the event, Error argument will be set
  to "true".  Note: Currently there is no guarantee that playEvent()
  will return immediately, since the functionality it invokes may enter
  a separate event loop (a modal dialog or context menu, for example). */
  void playEvent(
    const QString& Object,
    const QString& Command,
    const QString& Arguments,
    bool& Error);

private:
  pqEventPlayer(const pqEventPlayer&);
  pqEventPlayer& operator=(const pqEventPlayer&);
  

  /// Stores the working set of widget players  
  QList<pqWidgetEventPlayer*> Players;
};

#endif // !_pqEventPlayer_h
