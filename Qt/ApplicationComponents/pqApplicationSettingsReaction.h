// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqApplicationSettingsReaction_h
#define pqApplicationSettingsReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqSettingsDialog;

/**
 * @ingroup Reactions
 * pqApplicationSettingsReaction is a reaction to popup the application
 * settings dialog. It creates pqSettingsDialog when required.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqApplicationSettingsReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqApplicationSettingsReaction(QAction* parent);
  ~pqApplicationSettingsReaction() override;

  /**
   * Show the application settings dialog.
   */
  static void showApplicationSettingsDialog(const QString& tabName = "");

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqApplicationSettingsReaction::showApplicationSettingsDialog(); }

private:
  Q_DISABLE_COPY(pqApplicationSettingsReaction)

  static QPointer<pqSettingsDialog> Dialog;
};

#endif
