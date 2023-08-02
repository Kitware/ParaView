// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExampleVisualizationsDialogReaction_h
#define pqExampleVisualizationsDialogReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 *
 * pqExampleVisualizationsDialogReaction is used to show the
 * pqExampleVisualizationsDialog when the action is triggered.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqExampleVisualizationsDialogReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqExampleVisualizationsDialogReaction(QAction* parent);
  ~pqExampleVisualizationsDialogReaction() override;

  /**
   * Shows the example visualizations dialog.
   */
  static void showExampleVisualizationsDialog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override
  {
    pqExampleVisualizationsDialogReaction::showExampleVisualizationsDialog();
  }

private:
  Q_DISABLE_COPY(pqExampleVisualizationsDialogReaction)
};

#endif
