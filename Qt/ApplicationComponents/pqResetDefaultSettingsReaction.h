// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqResetDefaultSettingsReaction_h
#define pqResetDefaultSettingsReaction_h

#include "pqReaction.h"

#include <QStringList>

/**
 * @class pqResetDefaultSettingsReaction
 * @brief reaction to restore user settings to default.
 * @ingroup Reactions
 *
 * pqResetDefaultSettingsReaction can restore user settings to default. It
 * pops up a prompt indicating whether the user wants to generate backups for
 * settings being restored. If so, backups are generated.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqResetDefaultSettingsReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqResetDefaultSettingsReaction(QAction* parent);
  ~pqResetDefaultSettingsReaction() override;

  /**
   * Reset to default settings. Application must be restarted for the changes to
   * take effect.
   */
  virtual void resetSettingsToDefault();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->resetSettingsToDefault(); }

  virtual void clearSettings();
  virtual QStringList backupSettings();

private:
  Q_DISABLE_COPY(pqResetDefaultSettingsReaction)
};

#endif
