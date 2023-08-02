// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSetBreakpointDialog_h
#define pqSetBreakpointDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

namespace Ui
{
class pqSetBreakpointDialog;
}

class pqServer;
class QTreeWidget;
class pqPipelineSource;

/**
 * Sets a breakpoint for a remote simulation. It allows a user to
 * specify a time in the future (using simulation time or time step)
 * when a simulation linked with Catalyst should pause.
 *
 * @ingroup LiveInsitu
 */
class PQCOMPONENTS_EXPORT pqSetBreakpointDialog : public QDialog
{
  Q_OBJECT

public:
  pqSetBreakpointDialog(QWidget* Parent);
  ~pqSetBreakpointDialog() override;

Q_SIGNALS:
  void breakpointHit();

protected Q_SLOTS:
  void onAccepted();
  void onTimeUpdated();

private:
  Q_DISABLE_COPY(pqSetBreakpointDialog)
  Ui::pqSetBreakpointDialog* const Ui;
};

#endif // !pqSetBreakpointDialog_h
