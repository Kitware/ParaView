// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqXMLEventObserver_h
#define pqXMLEventObserver_h

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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Record on event in xml file
   */
  void onRecordEvent(const QString& Widget, const QString& Command, const QString& Arguments,
    const int& eventType) override;
};

#endif // !pqXMLEventObserver_h
