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
class vtkSMViewProxy;
class pqServer;

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
   * Called when the active view changes. Points the saved screenshot settings
   * to the new view and its layout, so that the user does not have to configure
   * them again. This is done after control returns to the event loop, because
   * a newly created view becomes active before it is assigned to a layout.
   */
  void onViewChanged();

  /**
   * Called before a server is removed. Clears the saved screenshot settings
   * if they belong to that server's session.
   */
  void onAboutToRemoveServer(pqServer*);

  /**
   * Called when the Settings action is triggered
   */
  void onSettings();

private:
  /**
   * Sets the View and Layout of shProxy to viewProxy and its layout. If the
   * captured view or layout changes, Save resets ImageResolution to its size.
   * If viewProxy is not in a layout, View is set to nullptr which disables save.
   */
  void ResetView(vtkSMSaveScreenshotProxy* shProxy, vtkSMViewProxy* viewProxy);
  void CopyProperties(vtkSMSaveScreenshotProxy* shProxySaved, vtkSMSaveScreenshotProxy* shProxy);

  Q_DISABLE_COPY(pqSaveStateAndScreenshotReaction);

  QString Directory;
  QString Name;
  vtkTypeUInt32 Location;
  bool FromCTest;
  // vtkSaveScreenshotProxy
  vtkSmartPointer<vtkSMProxy> Proxy;
  // ImageResolution is reset to the size of the captured view or layout on Save
  bool ResolutionFromTarget = false;
  QPointer<QAction> SettingsAction;
};

#endif
