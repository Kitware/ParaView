// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqProxyAction_h
#define pqProxyAction_h

#include "pqComponentsModule.h"

#include <QObject>
#include <QPointer>

class QAction;
class pqOutputPort;
class vtkSMProxy;

/**
 * \brief: a wrapper around a QAction used for proxy creation.
 *
 * \details: pqProxyAction can retrieve SMProxy information
 * from the QAction UserData, and also update the QAction
 * status tip depending on the active output ports.
 *
 * An action for a proxy is expected to store {proxyGroup, proxyName}
 * under QAction::data()
 */
class PQCOMPONENTS_EXPORT pqProxyAction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqProxyAction(QObject* parent, QAction* action);
  ~pqProxyAction() override;

  /**
   * Returns the underlying QAction.
   */
  QAction* GetAction();

  /**
   * Returns true if the action is enabled
   */
  bool IsEnabled();

  /**
   * Returns the display name, i.e. the text of the QAction.
   */
  QString GetDisplayName();

  /**
   * Returns the icon for this action.
   */
  QIcon GetIcon();

  /**
   * Returns the first unmet requirement (if any).
   * Also update the QAction status.
   */
  QString GetRequirement();

  /**
   * Returns the SMProxy Name.
   */
  QString GetProxyName();

  /**
   * Returns the SMProxy Group.
   */
  QString GetProxyGroup();

  /**
   * Returns the short documentation for the SMProxy.
   */
  QString GetDocumentation();

  /**
   * Returns the proxy documentation for the given action, if any.
   */
  static QString GetProxyDocumentation(QAction*);

  /**
   * Returns the proxy prototype for the given action, if any.
   */
  static vtkSMProxy* GetProxyPrototype(QAction*);

  /**
   * Returns the proxy name for the given action, if any.
   */
  static QString GetProxyName(QAction*);

  /**
   * Returns the proxy group for the given action, if any.
   */
  static QString GetProxyGroup(QAction*);

  /**
   * Update the state of the given actions.
   * This includes: enabled state and status tip (with unmet requirement).
   */
  static void updateActionsState(QList<QAction*> actions);

private:
  /**
   * Updates the action enabled state and status tip.
   */
  static void updateActionStatus(
    QAction* action, bool enabled, const QList<pqOutputPort*>& outputPorts);

  /**
   * Returns the active output ports.
   */
  static QList<pqOutputPort*> getOutputPorts();

  QPointer<QAction> Action;
};

#endif
