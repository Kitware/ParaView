/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqListPropertyWidget.h"

#include "vtkSMProperty.h"

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QItemDelegate>
#include <QLineEdit>
#include <QPointer>
#include <QTableWidget>

namespace
{
class pqListPropertyWidgetDelegate : public QItemDelegate
{
public:
  QPointer<QValidator> Validator;

  pqListPropertyWidgetDelegate(QObject* parentObject = nullptr)
    : QItemDelegate(parentObject)
  {
  }

  // create a line-edit with validator.
  QWidget* createEditor(QWidget* parentObject, const QStyleOptionViewItem& option,
    const QModelIndex& index) const override
  {
    (void)option;
    (void)index;
    QLineEdit* editor = new QLineEdit(parentObject);
    editor->setValidator(this->Validator);
    return editor;
  }
};
}

//-----------------------------------------------------------------------------
pqListPropertyWidget::pqListPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , TableWidget(new QTableWidget(this))
{
  pqListPropertyWidgetDelegate* delegate = new pqListPropertyWidgetDelegate(this);
  if (smproperty->IsA("vtkSMDoubleVectorProperty"))
  {
    delegate->Validator = new QDoubleValidator(delegate);
  }
  else if (smproperty->IsA("vtkSMIntVectorProperty"))
  {
    delegate->Validator = new QIntValidator(delegate);
  }

  this->TableWidget->setObjectName("ListWidget");
  this->TableWidget->setColumnCount(1);
  this->TableWidget->horizontalHeader()->setStretchLastSection(true);
  this->TableWidget->setItemDelegate(delegate);
  this->TableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

  QStringList headerLabels;
  headerLabels << smproperty->GetXMLLabel();
  this->TableWidget->setHorizontalHeaderLabels(headerLabels);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setMargin(0);
  hbox->setSpacing(0);
  hbox->addWidget(this->TableWidget);

  this->setShowLabel(false);
  this->addPropertyLink(this, "value", SIGNAL(valueChanged()), smproperty);

  QObject::connect(
    this->TableWidget, SIGNAL(itemChanged(QTableWidgetItem*)), this, SIGNAL(valueChanged()));
}

//-----------------------------------------------------------------------------
pqListPropertyWidget::~pqListPropertyWidget() = default;

//-----------------------------------------------------------------------------
QList<QVariant> pqListPropertyWidget::value() const
{
  QList<QVariant> values;
  for (int cc = 0; cc < this->TableWidget->rowCount(); cc++)
  {
    QTableWidgetItem* item = this->TableWidget->item(cc, 0);
    values << item->text();
  }
  return values;
}

//-----------------------------------------------------------------------------
void pqListPropertyWidget::setValue(const QList<QVariant>& values)
{
  this->TableWidget->setRowCount(values.size());
  for (int cc = 0; cc < values.size(); cc++)
  {
    QTableWidgetItem* item = this->TableWidget->item(cc, 0);
    if (item)
    {
      item->setText(values[cc].toString());
    }
    else
    {
      item = new QTableWidgetItem(values[cc].toString());
      item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
      this->TableWidget->setItem(cc, 0, item);
    }
  }
}
