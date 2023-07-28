// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqQuickLaunchDialog_h
#define pqQuickLaunchDialog_h

#include "pqWidgetsModule.h"

#include <QAction>
#include <QDialog>

/**
 * A borderless pop-up dialog used to show actions that the user can launch.
 * Provides search and quick apply capabilities.
 */
class PQWIDGETS_EXPORT pqQuickLaunchDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqQuickLaunchDialog(QWidget* parent = nullptr);
  ~pqQuickLaunchDialog() override;

  /**
   * Set the actions to be launched using this dialog.
   * This clears all already added actions.
   */
  void setActions(const QList<QAction*>& actions);

  /**
   * Add actions to be launched using this dialog.
   * This adds to already added actions.
   */
  void addActions(const QList<QAction*>& actions);

  /**
   * check if quick apply was set when accepting the dialog
   */
  bool quickApply();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Overridden to trigger the user selected action.
   */
  void accept() override;

protected Q_SLOTS:
  /**
   * Called when the user chooses an item from available choices shown in the
   * options list.
   */
  void currentRowChanged(int);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Overridden to capture key presses.
   */
  bool eventFilter(QObject* watched, QEvent* event) override;

  /**
   * Given the user entered text, update the GUI.
   */
  void updateSearch();

private:
  Q_DISABLE_COPY(pqQuickLaunchDialog)

  class pqInternal;
  pqInternal* Internal;
};

#endif
