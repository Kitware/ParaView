// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqProxiesActionListModel_h
#define pqProxiesActionListModel_h

#include "pqApplicationComponentsModule.h"

#include <QAbstractListModel>

class QAction;
class pqProxyAction;

/**
 * A model to handle a list of proxy action.
 *
 * Reimplements QAbstractListModel to provide a qt model for list of proxy action.
 * The different proxy informations can be accessed from dedicated item UserRole.
 *
 * Typically, Tooltip uses the proxy ShortHelp, and disabled actions are displayed
 * with a dedicated palette.
 *
 * Note: This is a private class and is not available outside of ParaView.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqProxyActionListModel : public QAbstractListModel
{
  Q_OBJECT
  typedef QAbstractListModel Superclass;

public:
  pqProxyActionListModel(const QList<QAction*>& proxyActions, QObject* parent = nullptr);
  ~pqProxyActionListModel() override;

  enum Role
  {
    DocumentationRole = Qt::UserRole + 1,
    ProxyEnabledRole,
    RequirementRole,
    ActionRole,
    GroupRole,
    NameRole
  };

  ///@{
  /**
   * Reimplements Superclass API to manage items.
   */
  int rowCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  ///@}

private:
  QList<pqProxyAction*> ProxyActions;
};

#endif
