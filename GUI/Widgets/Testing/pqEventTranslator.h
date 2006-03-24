/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

#ifndef _pqEventTranslator_h
#define _pqEventTranslator_h

#include <QObject>

class pqWidgetEventTranslator;

/// Manages translation of low-level Qt events to high-level ParaQ events that can be serialized as test-cases, demos, tutorials, etc.
class pqEventTranslator :
  public QObject
{
  Q_OBJECT

public:
  pqEventTranslator();
  ~pqEventTranslator();

  /// Adds the default set of widget translators to the working set.  Translators are executed in order, so you may call addWidgetEventTranslator() before this function to "override" the default translators
  void addDefaultWidgetEventTranslators();
  /// Adds a new translator to the current working set of widget translators.  pqEventTranslator assumes control of the lifetime of the supplied object.
  void addWidgetEventTranslator(pqWidgetEventTranslator*);

  /// Adds an object to a list of objects that should be ignored when translating events (useful to prevent recording UI events from being captured as part of the recording)
  void ignoreObject(QObject* Object);

signals:
  /// This signal will be emitted every time a translator generates a high-level ParaQ event.  Observers should connect to this signal to serialize high-level events.
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

