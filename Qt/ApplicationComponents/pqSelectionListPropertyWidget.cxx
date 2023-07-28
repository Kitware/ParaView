// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSelectionListPropertyWidget.h"
#include "ui_pqSelectionListPropertyWidget.h"

#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqSelectionInputWidget.h"
#include "pqSelectionManager.h"

#include <QCoreApplication>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QItemDelegate>
#include <QLineEdit>
#include <QPointer>
#include <QTableWidget>

class pqSelectionListPropertyWidget::pqUi : public Ui::pqSelectionListPropertyWidget
{
};

//-----------------------------------------------------------------------------
pqSelectionListPropertyWidget::pqSelectionListPropertyWidget(
  vtkSMProxy* smProxy, vtkSMPropertyGroup* smGroup, QWidget* parentObject)
  : Superclass(smProxy, smGroup, parentObject)
  , Ui(new pqSelectionListPropertyWidget::pqUi())
{
  // Set UI elements
  this->Ui->setupUi(this);

  this->Ui->labelOnSelection->setObjectName("SelectionListWidget");
  this->Ui->labelOnSelection->setColumnCount(1);
  this->Ui->labelOnSelection->horizontalHeader()->setStretchLastSection(true);
  this->Ui->labelOnSelection->setSelectionMode(QAbstractItemView::SingleSelection);

  QStringList verHeaderList({ "Labels" });
  this->Ui->labelOnSelection->setHorizontalHeaderLabels(verHeaderList);

  this->setShowLabel(true);

  // Set connection for the selection and the labels
  vtkSMProperty* smproperty = smGroup->GetProperty("Selection");
  if (smproperty)
  {
    this->addPropertyLink(
      this->Ui->SelectionInput, "selection", SIGNAL(selectionChanged(pqSMProxy)), smproperty);

    this->connect(
      this->Ui->SelectionInput, SIGNAL(selectionChanged(pqSMProxy)), SIGNAL(changeAvailable()));
    this->connect(
      this->Ui->SelectionInput, SIGNAL(selectionChanged(pqSMProxy)), SIGNAL(changeFinished()));

    this->connect(this->Ui->SelectionInput, SIGNAL(selectionChanged(pqSMProxy)),
      SLOT(populateRowLabels(pqSMProxy)));

    this->populateRowLabels(this->Ui->SelectionInput->selection());
  }
  else
  {
    this->Ui->SelectionInput->hide();
  }

  smproperty = smGroup->GetProperty("Labels");
  if (smproperty)
  {
    this->addPropertyLink(this, "labels", SIGNAL(labelsChanged()), smproperty);
    QObject::connect(this->Ui->labelOnSelection, SIGNAL(itemChanged(QTableWidgetItem*)), this,
      SIGNAL(labelsChanged()));
  }
  else
  {
    this->Ui->labelOnSelection->hide();
  }
}

//-----------------------------------------------------------------------------
pqSelectionListPropertyWidget::~pqSelectionListPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqSelectionListPropertyWidget::populateRowLabels(pqSMProxy appendSelection)
{
  // the selection source should be an appendSelections
  unsigned int numInputs = appendSelection && appendSelection->GetProperty("Input")
    ? vtkSMPropertyHelper(appendSelection, "Input").GetNumberOfElements()
    : 0;

  if (numInputs > 0)
  {
    this->Ui->labelOnSelection->setRowCount(numInputs);
    QList<QVariant> defaultValues;
    QStringList horHeaderList;
    for (unsigned int i = 0; i < numInputs; i++)
    {
      std::string str = "s" + std::to_string(i);
      QString name = QString::fromStdString(str);
      horHeaderList.push_back(name);
      defaultValues.push_back(QVariant());
    }

    this->Ui->labelOnSelection->setVerticalHeaderLabels(horHeaderList);
    this->setLabels(defaultValues);
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSelectionListPropertyWidget::labels() const
{
  QList<QVariant> values;
  for (int cc = 0; cc < this->Ui->labelOnSelection->rowCount(); cc++)
  {
    QTableWidgetItem* item = this->Ui->labelOnSelection->item(cc, 0);
    if (!item)
    {
      continue;
    }
    values << item->text();
  }
  return values;
}

//-----------------------------------------------------------------------------
void pqSelectionListPropertyWidget::setLabels(const QList<QVariant>& labels)
{
  if (labels.empty())
  {
    // Do nothing
    return;
  }

  this->Ui->labelOnSelection->setRowCount(labels.size());
  for (int cc = 0; cc < labels.size(); cc++)
  {
    QTableWidgetItem* item = this->Ui->labelOnSelection->item(cc, 0);
    if (item)
    {
      item->setText(labels[cc].toString());
    }
    else
    {
      item = new QTableWidgetItem(labels[cc].toString());
      item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
      this->Ui->labelOnSelection->setItem(cc, 0, item);
    }
  }
}
