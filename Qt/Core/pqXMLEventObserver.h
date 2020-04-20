/*=========================================================================

   Program: ParaView
   Module:    pqXMLEventObserver.h

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

#ifndef _pqXMLEventObserver_h
#define _pqXMLEventObserver_h

#include "pqCoreModule.h"
#include "pqEventObserver.h"

/**
Observes high-level ParaView events, and serializes them to a stream as XML
for possible playback (as a test-case, demo, tutorial, etc).  To use,
connect the onRecordEvent() slot to the pqEventTranslator::recordEvent()
signal.

\note Output is sent to the stream from this object's destructor, so you
must ensure that it goes out of scope before trying to playback the stream.

\sa pqEventTranslator, pqStdoutEventObserver, pqXMLEventSource.
*/

class PQCORE_EXPORT pqXMLEventObserver : public pqEventObserver
{
  Q_OBJECT

public:
  pqXMLEventObserver(QObject* p);
  ~pqXMLEventObserver() override;

  void setStream(QTextStream* stream) override;

public Q_SLOTS:
  /**
  * Record on event in xml file
  */
  void onRecordEvent(const QString& Widget, const QString& Command, const QString& Arguments,
    const int& eventType) override;

private:
};

#endif // !_pqXMLEventObserver_h
