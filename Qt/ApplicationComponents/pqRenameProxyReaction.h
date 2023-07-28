// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRenameProxyReaction_h
#define pqRenameProxyReaction_h

#include "pqReaction.h"

#include <QPointer>

class pqProxy;
class QWidget;

/**
 * @ingroup Reactions
 * Reaction for renaming a proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRenameProxyReaction : pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Use this overload to add a handler to rename a specific proxy.
   *
   * `parentWidget` is used as the parent for the dialog box popped up to
   * request new name input. If nullptr, pqCoreUtilities::mainWidget() is used.
   */
  pqRenameProxyReaction(QAction* renameAction, pqProxy* proxy, QWidget* parentWidget = nullptr);

  /**
   * Use this overload to add a handler to rename the active source proxy.
   *
   * `parentWidget` is used as the parent for the dialog box popped up to
   * request new name input. If nullptr, pqCoreUtilities::mainWidget() is used.
   */
  pqRenameProxyReaction(QAction* renameAction, QWidget* parentWidget = nullptr);

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;
  void updateEnableState() override;

protected: // NOLINT(readability-redundant-access-specifiers)
  pqProxy* Proxy;
  QPointer<QWidget> ParentWidget;
};

#endif
