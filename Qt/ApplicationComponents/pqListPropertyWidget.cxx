// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqListPropertyWidget.h"

#include "vtkSMProperty.h"

#include <QCoreApplication>
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
  headerLabels << QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel());
  this->TableWidget->setHorizontalHeaderLabels(headerLabels);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setContentsMargins(0, 0, 0, 0);
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
