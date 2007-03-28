/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorTreeWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include <QTreeWidget>

#include "pqTreeWidgetItemObject.h"

//-----------------------------------------------------------------------------
pqSignalAdaptorTreeWidget::pqSignalAdaptorTreeWidget(
  QTreeWidget* treeWidget, bool editable) : QObject(treeWidget)
{
  this->TreeWidget = treeWidget;
  this->Editable = editable;  
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
QTreeWidgetItem* pqSignalAdaptorTreeWidget::appendValue(
  const QList<QVariant> &value)
{
  int column_count = this->TreeWidget->columnCount();
  if (value.size() != column_count)
    {
    qDebug() << "Number of values does not match those required in one item.";
    return 0;
    }

  QStringList columnValues;
  for (int cc=0; cc <column_count; cc++)
    {
    columnValues << value[cc].toString();
    }

  pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(
      this->TreeWidget, columnValues);

  if (this->Editable)
    {
    item->setFlags(item->flags()| Qt::ItemIsEditable);
    QObject::connect(item, SIGNAL(modified()), 
      this, SIGNAL(valuesChanged()));
    }
  QObject::connect(item, SIGNAL(destroyed()), this, SIGNAL(valuesChanged()),
    Qt::QueuedConnection);
  return item;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorTreeWidget::setValues(const QList<QVariant>& new_values)
{
  int column_count = this->TreeWidget->columnCount();
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
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(
      this->TreeWidget, column_values);
    if (this->Editable)
      {
      item->setFlags(item->flags()| Qt::ItemIsEditable);
      QObject::connect(item, SIGNAL(modified()), 
        this, SIGNAL(valuesChanged()));
      }
    QObject::connect(item, SIGNAL(destroyed()), this, SIGNAL(valuesChanged()),
      Qt::QueuedConnection);
    }

  emit this->valuesChanged();
}
