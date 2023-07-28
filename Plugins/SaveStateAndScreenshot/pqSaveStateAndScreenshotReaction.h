// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveStateAndScreenshotReaction_h
#define pqSaveStateAndScreenshotReaction_h

#include "pqReaction.h"

#include "vtkSmartPointer.h"
#include <QPointer>
#include <QString>

class vtkSMProxy;
class vtkSMSaveScreenshotProxy;
class pqView;

/**
 * @ingroup Reactions
 * pqSaveStateAndScreenshotReaction is a reaction to Save State and Screenshot
 */
class pqSaveStateAndScreenshotReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqSaveStateAndScreenshotReaction(QAction* saveAction, QAction* settingsAction);
  ~pqSaveStateAndScreenshotReaction() override = default;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected Q_SLOTS:
  /**
   * Called when the Save action is triggered
   */
  void onTriggered() override;
  /**
   * Called on the active view changed to check if the layout changed.
   * If that happens, we deactivate the save button to force the user to save settings
   */
  void onViewChanged(pqView*);

  /**
   * Called when the Settings action is triggered
   */
  void onSettings();

private:
  void CopyProperties(vtkSMSaveScreenshotProxy* shProxySaved, vtkSMSaveScreenshotProxy* shProxy);

  Q_DISABLE_COPY(pqSaveStateAndScreenshotReaction);

  QString Directory;
  QString Name;
  vtkTypeUInt32 Location;
  bool FromCTest;
  // vtkSaveScreenshotProxy
  vtkSmartPointer<vtkSMProxy> Proxy;
  QPointer<QAction> SettingsAction;
};

#endif
