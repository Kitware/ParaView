// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginTreeWidgetEventPlayer.h"
#include "pqEventDispatcher.h"

#include "pqPluginTreeWidget.h"
#include "pqQtDeprecated.h"

#include <QDebug>
#include <QRegularExpression>
#include <QTreeWidget>

//-----------------------------------------------------------------------------
pqPluginTreeWidgetEventPlayer::pqPluginTreeWidgetEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqPluginTreeWidgetEventPlayer::~pqPluginTreeWidgetEventPlayer() = default;

namespace
{
//-----------------------------------------------------------------------------
QModelIndex pqPluginTreeWidgetEventPlayerGetIndex(
  const QString& str_index, QTreeView* treeView, bool& error)
{
  QStringList indices = str_index.split(".", PV_QT_SKIP_EMPTY_PARTS);
  QModelIndex index;
  for (int cc = 0; (cc + 1) < indices.size(); cc += 2)
  {
    index = treeView->model()->index(indices[cc].toInt(), indices[cc + 1].toInt(), index);
    if (!index.isValid())
    {
      error = true;
      qCritical() << "ERROR: Tree view must have changed. "
                  << "Indices recorded in the test are no longer valid. Cannot playback.";
      break;
    }
  }
  return index;
}

//-----------------------------------------------------------------------------
QModelIndex pqTreeViewEventPlayerGetIndexByColumnValue(
  const int column, const QString& columnValue, QTreeView* treeView, bool& error)
{
  int rows = treeView->model()->rowCount();
  for (int i = 0; i < rows; ++i)
  {
    QModelIndex index = treeView->model()->index(i, column, treeView->rootIndex());
    if (index.isValid())
    {
      QString value = index.data().toString();
      if (index.data().toString() == columnValue)
      {
        return index;
      }
    }
  }
  error = true;
  qCritical() << "ERROR: Failed to find column with data '" << columnValue << "'";
  return QModelIndex();
}
}

//-----------------------------------------------------------------------------0000000
bool pqPluginTreeWidgetEventPlayer::playEvent(
  QObject* object, const QString& command, const QString& arguments, bool& error)
{
  pqPluginTreeWidget* treeView = qobject_cast<pqPluginTreeWidget*>(object);
  if (!treeView)
  {
    return false;
  }

  QRegularExpression regExp0("^([\\d\\.]+),(\\d+),(\\d+)$");
  QRegularExpressionMatch match = regExp0.match(arguments);
  if (command == "setTreeItemCheckState" && match.hasMatch())
  {
    // legacy command recorded from tree widgets.
    QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
    if (!treeWidget)
    {
      return false;
    }
    QString str_index = match.captured(1);
    int column = match.captured(2).toInt();
    int check_state = match.captured(3).toInt();

    QStringList indices = str_index.split(".", PV_QT_SKIP_EMPTY_PARTS);
    QTreeWidgetItem* cur_item = nullptr;
    Q_FOREACH (QString cur_index, indices)
    {
      int index = cur_index.toInt();
      if (!cur_item)
      {
        cur_item = treeWidget->topLevelItem(index);
      }
      else
      {
        cur_item = cur_item->child(index);
      }
      if (!cur_item)
      {
        error = true;
        qCritical() << "ERROR: Tree widget must have changed. "
                    << "Indices recorded in the test are no longer valid. Cannot playback.";
        return true;
      }
    }
    cur_item->setCheckState(column, static_cast<Qt::CheckState>(check_state));
    return true;
  }

  QRegularExpression regExp1("^([\\d\\.]+),(\\d+)$");
  match = regExp1.match(arguments);
  if (command == "setCheckState" && match.hasMatch())
  {
    const QString& str_index = match.captured(1);
    int check_state = match.captured(2).toInt();

    QModelIndex index = ::pqPluginTreeWidgetEventPlayerGetIndex(str_index, treeView, error);
    if (error)
    {
      return true;
    }
    if (treeView->model()->data(index, Qt::CheckStateRole).toInt() != check_state)
    {
      treeView->model()->setData(
        index, static_cast<Qt::CheckState>(check_state), Qt::CheckStateRole);
    }
    return true;
  }
  else if (command == "expand" || command == "collapse")
  {
    const QString& str_index = arguments;
    QModelIndex index = ::pqPluginTreeWidgetEventPlayerGetIndex(str_index, treeView, error);
    if (error)
    {
      return true;
    }
    treeView->setExpanded(index, (command == "expand"));
    return true;
  }
  else if (command == "setCurrent")
  {
    const QString& columnValue = arguments;
    QModelIndex index =
      ::pqTreeViewEventPlayerGetIndexByColumnValue(0, columnValue, treeView, error);
    if (error)
    {
      return true;
    }
    treeView->setCurrentIndex(index);

    return true;
  }
  return false;
}
