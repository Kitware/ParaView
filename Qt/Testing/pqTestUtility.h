/*=========================================================================

   Program: ParaView
   Module:    pqTestUtility.h

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

#ifndef _pqTestUtility_h
#define _pqTestUtility_h

#include <QObject>
#include <QMap>
#include <QSet>
#include <QTextStream>
#include <QFile>

#include "QtTestingExport.h"
#include "pqEventDispatcher.h"
#include "pqEventPlayer.h"
#include "pqEventTranslator.h"
class pqEventObserver;
class pqEventSource;

/// Organizes basic functionality for regression testing
class QTTESTING_EXPORT pqTestUtility :
  public QObject
{
  Q_OBJECT

public:
  pqTestUtility(QObject* parent = 0);
  ~pqTestUtility();

public:

  /// get the event dispatcher
  pqEventDispatcher* dispatcher();
  /// get the event player
  pqEventPlayer* eventPlayer();
  /// get the event translator
  pqEventTranslator* eventTranslator();

  /// add an event source for playback of files
  /// An pqXMLEventSource is automatically added if XML support is enabled.
  /// A pqPythonEventSource is automatically added if Python support is enabled.
  void addEventSource(const QString& fileExtension, pqEventSource* source);
  
  /// add an event observer for recording files
  /// An pqXMLEventObserver is automatically added if XML support is enabled.
  /// A pqPythonEventObserver is automatically added if Python support is enabled.
  void addEventObserver(const QString& fileExtension, 
                          pqEventObserver* translator);
  
public slots:
  /// start the playing of tests in a file
  virtual void playTests(const QString& filename);

  /// start the recording of tests to a file
  virtual void recordTests(const QString& filename);

protected slots:
  virtual void testSucceeded();
  virtual void testFailed();

protected:
  pqEventDispatcher Dispatcher;
  pqEventPlayer Player;
  pqEventTranslator Translator;

  QMap<QString, pqEventSource*> EventSources;
  QMap<QString, pqEventObserver*> EventObservers;
};

#endif // !_pqTestUtility_h

