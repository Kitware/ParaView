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

class pqEventPlayer;
class pqEventSource;

class QTTESTING_EXPORT pqEventDispatcher :
  public QObject
{
  Q_OBJECT
  
public:
  pqEventDispatcher();
  ~pqEventDispatcher();

  /** Retrieves events from the given event source, dispatching them to
  the given event player for test case playback.  Note that playback is
  asynchronous - the call to playEvents() returns immediately.  Callers
  must ensure that the source, dispatcher, and player objects remain
  in-scope until either the succeeded() or failed() signal is emitted
  to indicate that playback has finished. */
  void playEvents(pqEventSource& source, pqEventPlayer& player);

  /** Wait function provided for players that need to wait for the GUI
      to perform a certain action */
  static void processEventsAndWait(int ms);

signals:
  void succeeded();
  void failed();
  void readyPlayNextEvent();

private slots:
  void playNextEvent();
  void checkPlayNextEvent();
  void queueNextEvent();

private:
  void stopPlayback();

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqEventDispatcher_h
