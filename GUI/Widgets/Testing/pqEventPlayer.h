/*=========================================================================

   Program:   ParaQ
   Module:    pqEventPlayer.h

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

#ifndef _pqEventPlayer_h
#define _pqEventPlayer_h

#include "QtTestingExport.h"

#include <QString>
#include <QVector>

class QObject;
class pqWidgetEventPlayer;

/// Manages translation of high-level ParaQ events to low-level Qt events, for playback of test-cases, demos, tutorials, etc.
class QTTESTING_EXPORT pqEventPlayer
{
public:
  pqEventPlayer();
  ~pqEventPlayer();

  /// Adds the default set of widget players to the current working set.  Players are executed in-order, so you can call addWidgetEventPlayer() before this function to override default players.
  void addDefaultWidgetEventPlayers();
  /// Adds a new player to the current working set of widget players.  pqEventPlayer assumes control of the lifetime of the supplied object.
  void addWidgetEventPlayer(pqWidgetEventPlayer*);

  /// This method is called with each high-level ParaQ event, which will invoke the corresponding low-level Qt functionality in-turn.  Returns true on success, false on failure.
  bool playEvent(const QString& Object, const QString& Command, const QString& Arguments = QString());

private:
  pqEventPlayer(const pqEventPlayer&);
  pqEventPlayer& operator=(const pqEventPlayer&);

  /// Stores the working set of widget players  
  QVector<pqWidgetEventPlayer*> Players;
};

#endif // !_pqEventPlayer_h

