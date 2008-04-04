/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorTreeWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqSignalAdaptorTreeWidget.h"

#include <QtDebug>
#include <QHeaderView>

#include "pqTreeWidget.h"
#include "pqTreeWidgetItemObject.h"

//-----------------------------------------------------------------------------
pqSignalAdaptorTreeWidget::pqSignalAdaptorTreeWidget(
  QTreeWidget* treeWidget, bool editable) : QObject(treeWidget)
{
  this->TreeWidget = treeWidget;
  this->Sortable = this->TreeWidget->isSortingEnabled();
  this->Editable = editable;  
  this->ItemCreatorFunctionPtr = 0;

  // by default none nothing is sorted until the user clicks on one of the
  // headers.
  this->TreeWidget->setSortingEnabled(false);
  this->updateSortingLinks();
  
  if (editable)
    {
    pqTreeWidget* pqtree = qobject_cast<pqTreeWidget*>(treeWidget);
    if (pqtree)
      {
      // When user is manually editing the indices, we want the table to be able
      // to grow as the user goes beyond the end.
      QObject::connect(pqtree, SIGNAL(navigatedPastEnd()),
        this, SLOT(growTable()));
      }
    }
}

//-----------------------------------------------------------------------------
pqSignalAdaptorTreeWidget::~pqSignalAdaptorTreeWidget()
{
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSignalAdaptorTreeWidget::values() const
{
  QList<QVariant> reply;

  int max = this->TreeWidget->topLevelItemCount();
  int column_count = this->TreeWidget->columnCount();
  for (int cc=0; cc < max; cc++)
    {
    QTreeWidgetItem* item = this->TreeWidget->topLevelItem(cc);
    if (item)
      {
      for (int kk=0; kk <column_count; kk++)
        {
        reply.push_back(item->text(kk));
        }
      }
    }
  return reply;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorTreeWidget::appendItem(pqTreeWidgetItemObject* item)
{
  this->TreeWidget->addTopLevelItem(item);

  if (this->Editable)
    {
    item->setFlags(item->flags()| Qt::ItemIsEditable);
    QObject::connect(item, SIGNAL(modified()), 
      this, SIGNAL(valuesChanged()));
    }
  QObject::connect(item, SIGNAL(destroyed()), this, SIGNAL(valuesChanged()),
    Qt::QueuedConnection);

  emit this->valuesChanged();
}

//-----------------------------------------------------------------------------
pqTreeWidgetItemObject* pqSignalAdaptorTreeWidget::appendValue(
  const QStringList& columnValues)
{
  int column_count = this->TreeWidget->columnCount();
  if (columnValues.size() != column_count)
    {
    qDebug() << "Number of values does not match those required in one item.";
    return 0;
    }

  pqTreeWidgetItemObject* item = NULL;
  if (this->ItemCreatorFunctionPtr)
    {
    item = (*this->ItemCreatorFunctionPtr)(NULL, columnValues);
    }

  if (!item)
    {
    item = new pqTreeWidgetItemObject(columnValues);
    }

  this->appendItem(item);
  return item;
}

//-----------------------------------------------------------------------------
pqTreeWidgetItemObject* pqSignalAdaptorTreeWidget::appendValue(
  const QList<QVariant> &value)
{
  QStringList strVals;
  foreach (QVariant v, value)
    {
    strVals.push_back(v.toString());
    }

  return this->appendValue(strVals);
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorTreeWidget::setValues(const QList<QVariant>& new_values)
{
  this->blockSignals(true);
  int column_count = this->TreeWidget->columnCount();

  QList<QTreeWidgetItem*> items;
  // Disconnect all items first.
  int num_items = this->TreeWidget->topLevelItemCount();
  for (int cc=0; cc < num_items; cc++)
    {
    QObject::disconnect(
      dynamic_cast<pqTreeWidgetItemObject*>(this->TreeWidget->topLevelItem(cc)), 0, 
      this, 0);
    }
  this->TreeWidget->clear();

  if (new_values.size()%column_count != 0)
    {
    qDebug() << "Uneven size for values.";
    }

  for (int cc=0; (cc+column_count) <= new_values.size(); cc+=column_count)
    {
    QStringList column_values;
    for (int i=0; i < column_count; i++)
      {
      column_values.push_back(new_values[cc+i].toString());
      }

    pqTreeWidgetItemObject* item = NULL;
    if (this->ItemCreatorFunctionPtr)
      {
      item = (*this->ItemCreatorFunctionPtr)(NULL, column_values);
      }

    if (!item)
      {
      item  = new pqTreeWidgetItemObject(column_values);
      }

    if (this->Editable)
      {
      item->setFlags(item->flags()| Qt::ItemIsEditable);
      QObject::connect(item, SIGNAL(modified()), 
        this, SIGNAL(valuesChanged()));
      }
    QObject::connect(item, SIGNAL(destroyed()), this, SIGNAL(valuesChanged()), 
      Qt::QueuedConnection);
    items.push_back(item);
    }
  this->TreeWidget->addTopLevelItems(items);
  this->blockSignals(false);
  emit this->valuesChanged();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorTreeWidget::updateSortingLinks()
{
  if (this->Sortable && !this->TreeWidget->isSortingEnabled())
    {
    QObject::connect(this->TreeWidget->header(),
      SIGNAL(sectionClicked(int)), 
      this, SLOT(sort(int)));
    this->TreeWidget->header()->setClickable(true);
    }
  else
    {
    QObject::disconnect(this->TreeWidget->header(), 0, this, 0);
    }
}


//-----------------------------------------------------------------------------
void pqSignalAdaptorTreeWidget::sort(int column)
{
  if (!this->TreeWidget->isSortingEnabled())
    {
    this->TreeWidget->setSortingEnabled(this->Sortable);
    this->TreeWidget->sortByColumn(column);
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorTreeWidget::growTable()
{
  this->TreeWidget->setSortingEnabled(false);
  int columnCount = this->TreeWidget->columnCount();

  QTreeWidgetItem* sample = 0;
  if (this->TreeWidget->topLevelItemCount()>0)
    {
    sample = this->TreeWidget->topLevelItem(this->TreeWidget->topLevelItemCount()-1);
    }

  QStringList newvalue;
  for (int cc=0; cc < columnCount; cc++)
    {
    if (sample)
      {
      newvalue.push_back(sample->text(cc));
      }
    else
      {
      newvalue.push_back("0");
      }
    }

  bool prev = this->blockSignals(true);
  pqTreeWidgetItemObject* item = this->appendValue(newvalue);
  this->blockSignals(prev);

  // Give the listeners a chance to change item default values.
  emit this->tableGrown(item);


  // This ensures that when the user is finshed editing the value, it suddenly
  // doesn't move about due to sorting. The user can sort columns again once
  // he's done filling values by clicking on the header.
  this->updateSortingLinks();
  emit this->valuesChanged();
}
