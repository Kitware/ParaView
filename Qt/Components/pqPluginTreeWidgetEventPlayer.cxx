/*=========================================================================

   Program: ParaView
   Module:    pqPluginTreeWidgetEventPlayer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqPluginTreeWidgetEventPlayer.h"
#include "pqEventDispatcher.h"

#include "pqPluginTreeWidget.h"
#include "pqQtDeprecated.h"

#include <QDebug>
#include <QTreeWidget>

//-----------------------------------------------------------------------------
pqPluginTreeWidgetEventPlayer::pqPluginTreeWidgetEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqPluginTreeWidgetEventPlayer::~pqPluginTreeWidgetEventPlayer() = default;

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

//-----------------------------------------------------------------------------0000000
bool pqPluginTreeWidgetEventPlayer::playEvent(
  QObject* object, const QString& command, const QString& arguments, bool& error)
{
  pqPluginTreeWidget* treeView = qobject_cast<pqPluginTreeWidget*>(object);
  if (!treeView)
  {
    return false;
  }

  QRegExp regExp0("^([\\d\\.]+),(\\d+),(\\d+)$");
  if (command == "setTreeItemCheckState" && regExp0.indexIn(arguments) != -1)
  {
    // legacy command recorded from tree widgets.
    QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
    if (!treeWidget)
    {
      return false;
    }
    QString str_index = regExp0.cap(1);
    int column = regExp0.cap(2).toInt();
    int check_state = regExp0.cap(3).toInt();

    QStringList indices = str_index.split(".", PV_QT_SKIP_EMPTY_PARTS);
    QTreeWidgetItem* cur_item = nullptr;
    foreach (QString cur_index, indices)
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

  QRegExp regExp1("^([\\d\\.]+),(\\d+)$");
  if (command == "setCheckState" && regExp1.indexIn(arguments) != -1)
  {
    QString str_index = regExp1.cap(1);
    int check_state = regExp1.cap(2).toInt();

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
    QString str_index = arguments;
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
    QString columnValue = arguments;
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
