/*=========================================================================

   Program: ParaView
   Module:    pqEventTranslator.h

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

#ifndef _pqEventTranslator_h
#define _pqEventTranslator_h

#include "QtTestingExport.h"
#include <QObject>

class pqWidgetEventTranslator;

/**
Manages serialization of user interaction for test-cases, demos, tutorials, etc.
pqEventTranslator installs itself as a global Qt event "filter" that receives notification of every Qt event.  Each event is passed
through a collection of pqWidgetEventTranslator objects, until one of them "handles" the event.  The pqWidgetEventTranslator objects
convert low-level Qt events (mouse move, button down, key released, etc) into high-level ParaView events (button clicked, row selected, etc)
that can be serialized as text.  Once an event translator is found, the recordEvent() signal is emitted with the name of the widget
that is receiving the event, plus the serialized event.  Observers such as pqEventObserverXML connect to the recordEvent() signal and
handle storage of the events.

\sa pqWidgetEventTranslator, pqEventObserverStdout, pqEventObserverXML, pqEventPlayer.
*/
class QTTESTING_EXPORT pqEventTranslator :
  public QObject
{
  Q_OBJECT

public:
  pqEventTranslator(QObject* p=0);
  ~pqEventTranslator();

  /**
  Adds the default set of widget translators to the working set.  Translators are executed in order, so you may call
  addWidgetEventTranslator() with your own custom translators before calling this method, to "override" the default translators.
  */
  void addDefaultWidgetEventTranslators();
  /// Adds a new translator to the current working set of widget translators.  pqEventTranslator assumes control of the lifetime of the supplied object.
  void addWidgetEventTranslator(pqWidgetEventTranslator*);

  /// Adds a Qt object to a list of objects that should be ignored when translating events (useful to prevent recording UI events from being captured as part of the recording)
  void ignoreObject(QObject* Object);

  /// start listening to the GUI and translating events
  void start();
  
  /// stop listening to the GUI and translating events
  void stop();

signals:
  /// This signal will be emitted every time a translator generates a high-level ParaView event.  Observers should connect to this signal to serialize high-level events.
  void recordEvent(const QString& Object, const QString& Command, const QString& Arguments);

private slots:
  void onRecordEvent(QObject* Object, const QString& Command, const QString& Arguments);
  
private:
  pqEventTranslator(const pqEventTranslator&);
  pqEventTranslator& operator=(const pqEventTranslator&);

  bool eventFilter(QObject* Object, QEvent* Event);

  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqEventTranslator_h
