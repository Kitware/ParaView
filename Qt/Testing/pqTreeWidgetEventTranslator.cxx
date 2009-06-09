/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidgetEventTranslator.cxx

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
#include "pqTreeWidgetEventTranslator.h"

#include <QTreeWidget>
#include <QEvent>

//-----------------------------------------------------------------------------
pqTreeWidgetEventTranslator::pqTreeWidgetEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqTreeWidgetEventTranslator::~pqTreeWidgetEventTranslator()
{
}

//-----------------------------------------------------------------------------
bool pqTreeWidgetEventTranslator::translateEvent(
  QObject* object, QEvent* tr_event, bool& /*error*/)
{
  QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
  if(!treeWidget)
    {
    // mouse events go to the viewport widget
    treeWidget = qobject_cast<QTreeWidget*>(object->parent());
    }
  if(!treeWidget)
    {
    return false;
    }

  if (tr_event->type() == QEvent::FocusIn)
    {
    QObject::disconnect(treeWidget, 0, this, 0);
    QObject::connect(treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
      this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
    }
  return true;
}

//-----------------------------------------------------------------------------
void pqTreeWidgetEventTranslator::onItemChanged(
  QTreeWidgetItem* item, int column)
{
  QTreeWidget* treeWidget = item->treeWidget();

  QTreeWidgetItem* cur_item = item;
  QString str_index;
  while (cur_item)
    {
    QTreeWidgetItem* parentItem = cur_item->parent();
    if (parentItem)
      {
      str_index.prepend(QString("%1.").arg(parentItem->indexOfChild(cur_item)));
      }
    else
      {
      str_index.prepend(QString("%1.").arg(treeWidget->indexOfTopLevelItem(cur_item)));
      }
    cur_item = parentItem;
    }

  // remove the last ".".
  str_index.chop(1);

  emit this->recordEvent( treeWidget, "setTreeItemCheckState",
    QString("%1,%2,%3").arg(str_index).arg(column).arg(
      static_cast<int>(item->checkState(column))));
}
