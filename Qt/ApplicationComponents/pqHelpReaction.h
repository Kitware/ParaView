// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHelpReaction_h
#define pqHelpReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * pqHelpReaction is reaction to show application help using Qt assistant.
 * It searches in ":/<AppName>HelpCollection/" for "*.qhc" files and shows the
 * first help collection file found as the help collection.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqHelpReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqHelpReaction(QAction* parent);

  /**
   * Show help for the application.
   */
  static void showHelp();

  /**
   * Show a particular help page.
   */
  static void showHelp(const QString& url);

  /**
   * Show the documentation for a particular proxy.
   */
  static void showProxyHelp(const QString& group, const QString& name);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqHelpReaction::showHelp(); }

private:
  Q_DISABLE_COPY(pqHelpReaction)
};

#endif
