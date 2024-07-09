// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqProxyActionListModel.h"

#include "pqProxyAction.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QPalette>
#include <QVariant>

//-----------------------------------------------------------------------------
pqProxyActionListModel::pqProxyActionListModel(const QList<QAction*>& proxyActions, QObject* parent)
  : QAbstractListModel(parent)
{
  for (QAction* action : proxyActions)
  {
    this->ProxyActions << new pqProxyAction(this, action);
  }
}

//-----------------------------------------------------------------------------
pqProxyActionListModel::~pqProxyActionListModel() = default;

//-----------------------------------------------------------------------------
int pqProxyActionListModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return this->ProxyActions.size();
}

//-----------------------------------------------------------------------------
QVariant pqProxyActionListModel::data(const QModelIndex& index, int role) const
{
  pqProxyAction* proxyAction = this->ProxyActions[index.row()];
  switch (role)
  {
    case Qt::DisplayRole:
    {
      return proxyAction->GetDisplayName();
    }
    break;
    case Qt::DecorationRole:
    {
      return proxyAction->GetIcon();
    }
    break;
    case Qt::ForegroundRole:
    {
      auto palette = QApplication::palette();
      if (proxyAction->IsEnabled())
      {
        return palette.text();
      }
      else
      {
        return palette.placeholderText();
      }
    }
    break;
    case Qt::ToolTipRole:
    {
      QString tooltip = proxyAction->GetDocumentation();
      if (!proxyAction->IsEnabled())
      {
        tooltip += "\n";
        tooltip += proxyAction->GetRequirement();
      }
      return tooltip;
    }
    break;
    case DocumentationRole:
    {
      return proxyAction->GetDocumentation();
    }
    break;
    case ProxyEnabledRole:
    {
      return proxyAction->IsEnabled();
    }
    break;
    case RequirementRole:
    {
      if (!proxyAction->IsEnabled())
      {
        return proxyAction->GetRequirement();
      }
      else
      {
        return QString();
      }
    }
    break;
    case ActionRole:
    {
      return QVariant::fromValue(proxyAction->GetAction());
    }
    break;
    case GroupRole:
    {
      return proxyAction->GetProxyGroup();
    }
    break;
    case NameRole:
    {
      return proxyAction->GetProxyName();
    }
    break;
    default:
      break;
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqProxyActionListModel::headerData(int, Qt::Orientation, int) const
{
  return tr("Proxy");
}
