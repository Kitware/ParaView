// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLogViewerReaction_h
#define pqLogViewerReaction_h

#include <pqReaction.h>

/**
 * @ingroup Reactions
 * Reaction for showing the log viewer.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLogViewerReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqLogViewerReaction(QAction* parentObject)
    : Superclass(parentObject)
  {
  }
  /**
   * Pops up (or raises) the log viewer.
   */
  static void showLogViewer();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqLogViewerReaction::showLogViewer(); }

private:
  Q_DISABLE_COPY(pqLogViewerReaction)
};

#endif // pqLogViewerReaction_h
