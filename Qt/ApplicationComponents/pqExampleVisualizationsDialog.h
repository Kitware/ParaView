// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExampleVisualizationsDialog_h
#define pqExampleVisualizationsDialog_h

#include <QDialog>

#include "pqApplicationComponentsModule.h"

namespace Ui
{
class pqExampleVisualizationsDialog;
}

/**
 * pqExampleVisualizationsDialog is a dialog used to show available example
 * visualizations to the user. The user can select one of the examples and load
 * them. This requires that ParaView tutorial data is available at specific
 * locations relative to the executable (see implementation for details).
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqExampleVisualizationsDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  explicit pqExampleVisualizationsDialog(QWidget* parent = nullptr);
  ~pqExampleVisualizationsDialog() override;

protected Q_SLOTS:
  virtual void onButtonPressed();

private:
  Ui::pqExampleVisualizationsDialog* ui;
};

#endif // pqExampleVisualizationsDialog_h
