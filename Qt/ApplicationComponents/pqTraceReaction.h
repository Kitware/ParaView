// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqTraceReaction(QAction* parent, QString start_trace_label = "Start Trace",
    QString stop_trace_label = "Stop Trace");
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

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void updateTrace();

private:
  Q_DISABLE_COPY(pqTraceReaction)
  QString StartTraceLabel;
  QString StopTraceLabel;

  void editTrace(const QString& txt, bool incremental);

  bool AutoSavePythonEnabled = false;
};

#endif
