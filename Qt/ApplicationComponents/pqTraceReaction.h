/*=========================================================================

   Program: ParaView
   Module:    pqTraceReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqTraceReaction_h
#define pqTraceReaction_h

#include "pqReaction.h"

#include <QPointer> // for QPointer.

class pqPythonScriptEditor;

/**
* @ingroup Reactions
* Reaction for application python start/stop trace. This reaction will change the
* label on the QAction to reflect whether the trace is started or stopped.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqTraceReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqTraceReaction(QAction* parent, const char* start_trace_label = "Start Trace",
    const char* stop_trace_label = "Stop Trace");
  ~pqTraceReaction() override;

  /**
  * start tracing.
  */
  void start();

  /**
  * stop tracing.
  */
  void stop();

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

protected Q_SLOTS:
  virtual void updateTrace();

private:
  Q_DISABLE_COPY(pqTraceReaction)
  QString StartTraceLabel;
  QString StopTraceLabel;

  void editTrace(const QString& txt, bool incremental);
};

#endif
