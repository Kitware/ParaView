/*=========================================================================

   Program: ParaView
   Module:    pqEventDispatcher.h

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

#ifndef _pqEventDispatcher_h
#define _pqEventDispatcher_h

#include "QtTestingExport.h"

#include <QObject>
#include <QTimer>
#include <QEvent>

class pqEventPlayer;
class pqEventSource;

/// pqEventDispatcher is responsible for taking each "event" from the test and
/// then "playing" it using the player. The dispatcher is the critical component
/// of this playback since it decides when it's time to dispatch the next
/// "event" from the test.
class QTTESTING_EXPORT pqEventDispatcher : public QObject
{
  Q_OBJECT
  typedef QObject Superclass; 
public:
  pqEventDispatcher(QObject* parent=0);
  ~pqEventDispatcher();

  /// Retrieves events from the given event source, dispatching them to
  /// the given event player for test case playback. This call blocks until all
  /// the events from the source have been played back (or failure). Returns
  /// true if playback was successful.
  bool playEvents(pqEventSource& source, pqEventPlayer& player);

  /** Wait function provided for players that need to wait for the GUI
      to perform a certain action */
  static void processEventsAndWait(int ms);

protected:
  /// filter application level events. This is not really a "filter", but more
  /// like an observer of the application level events.
  bool eventFilter(QObject *obj, QEvent *ev); 

signals:
  void triggerPlayEventStack(void*);

protected slots:
  /// Plays event set, until 
  /// 2> All events have been processed
  /// 3> There's an error.
  void playEventStack(void* activeWidget);

  void onMenuTimerTimeout();
protected:
  bool PlayBackFinished;
  bool PlayBackStatus;
  static bool DeferMenuTimeouts;

  pqEventSource* ActiveSource;
  pqEventPlayer* ActivePlayer;
  QTimer AdhocMenuTimer;
  QList<QWidget*> ActiveModalWidgetStack;
};

#endif // !_pqEventDispatcher_h
