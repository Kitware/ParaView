// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPluginTreeWidgetEventTranslator.h"

#include "pqPluginTreeWidget.h"

#include <QEvent>
#include <QTreeView>

//-----------------------------------------------------------------------------
pqPluginTreeWidgetEventTranslator::pqPluginTreeWidgetEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqPluginTreeWidgetEventTranslator::~pqPluginTreeWidgetEventTranslator() = default;

//-----------------------------------------------------------------------------
bool pqPluginTreeWidgetEventTranslator::translateEvent(
  QObject* object, QEvent* tr_event, bool& /*error*/)
{
  pqPluginTreeWidget* treeWidget = qobject_cast<pqPluginTreeWidget*>(object);
  if (!treeWidget)
  {
    // mouse events go to the viewport widget
    treeWidget = qobject_cast<pqPluginTreeWidget*>(object->parent());
  }
  if (!treeWidget)
  {
    return false;
  }

  if (tr_event->type() == QEvent::FocusIn)
  {
    if (this->TreeView)
    {
      QObject::disconnect(this->TreeView, nullptr, this, nullptr);
      QObject::disconnect(this->TreeView->selectionModel(), nullptr, this, nullptr);
    }

    QObject::connect(treeWidget, SIGNAL(clicked(const QModelIndex&)), this,
      SLOT(onItemChanged(const QModelIndex&)));
    QObject::connect(
      treeWidget, SIGNAL(expanded(const QModelIndex&)), this, SLOT(onExpanded(const QModelIndex&)));
    QObject::connect(treeWidget, SIGNAL(collapsed(const QModelIndex&)), this,
      SLOT(onCollapsed(const QModelIndex&)));
    QObject::connect(treeWidget->selectionModel(),
      SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
      SLOT(onCurrentChanged(const QModelIndex&)));
    this->TreeView = treeWidget;
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqPluginTreeWidgetEventTranslator::onItemChanged(const QModelIndex& index)
{
  QTreeView* treeWidget = qobject_cast<QTreeView*>(this->sender());
  QString str_index = this->getIndexAsString(index);
  if ((index.model()->flags(index) & Qt::ItemIsUserCheckable) != 0)
  {
    // record the check state change if the item is user-checkable.
    Q_EMIT this->recordEvent(treeWidget, "setCheckState",
      QString("%1,%3").arg(str_index).arg(index.model()->data(index, Qt::CheckStateRole).toInt()));
  }
}

//-----------------------------------------------------------------------------
void pqPluginTreeWidgetEventTranslator::onExpanded(const QModelIndex& index)
{
  QTreeView* treeWidget = qobject_cast<QTreeView*>(this->sender());

  // record the check state change if the item is user-checkable.
  Q_EMIT this->recordEvent(treeWidget, "expand", this->getIndexAsString(index));
}

//-----------------------------------------------------------------------------
void pqPluginTreeWidgetEventTranslator::onCollapsed(const QModelIndex& index)
{
  QTreeView* treeWidget = qobject_cast<QTreeView*>(this->sender());

  // record the check state change if the item is user-checkable.
  Q_EMIT this->recordEvent(treeWidget, "collapse", this->getIndexAsString(index));
}

//-----------------------------------------------------------------------------
QString pqPluginTreeWidgetEventTranslator::getIndexAsString(const QModelIndex& index)
{
  QModelIndex curIndex = index;
  QString str_index;
  while (curIndex.isValid())
  {
    str_index.prepend(QString("%1.%2.").arg(curIndex.row()).arg(curIndex.column()));
    curIndex = curIndex.parent();
  }

  // remove the last ".".
  str_index.chop(1);
  return str_index;
}

//-----------------------------------------------------------------------------
void pqPluginTreeWidgetEventTranslator::onCurrentChanged(const QModelIndex& index)
{
  pqPluginTreeWidget* treeWidget = this->TreeView;

  if (treeWidget && index.isValid())
  {
    QTreeWidgetItem* currentItem = treeWidget->currentItem();
    if (currentItem)
    {
      Q_EMIT this->recordEvent(treeWidget, "setCurrent", currentItem->text(0));
    }
  }
}
