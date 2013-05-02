/*=========================================================================

   Program: ParaView
   Module: pqScalarValueListPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#include "pqScalarValueListPropertyWidget.h"

#include <cmath>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "pqSMAdaptor.h"
#include "pqCollapsedGroup.h"
#include "pqSampleScalarAddRangeDialog.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMProperty.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkWeakPointer.h"

class pqScalarValueListPropertyWidgetPrivate
{
public:
    QListWidget *ListWidget;
    QLabel* ScalarRange;

    vtkNew<vtkEventQtSlotConnect> VTKRangeConnector;
    vtkWeakPointer<vtkSMDoubleRangeDomain> RangeDomain;
};

pqScalarValueListPropertyWidget::pqScalarValueListPropertyWidget(vtkSMProperty *smProperty,
                                                                 vtkSMProxy *smProxy,
                                                                 QWidget *pWidget)
  : pqPropertyWidget(smProxy, pWidget),
    d(new pqScalarValueListPropertyWidgetPrivate)
{
  this->setShowLabel(false);

  QVBoxLayout *groupBoxLayout = new QVBoxLayout;
  QHBoxLayout *groupListLayout = new QHBoxLayout;
  d->ListWidget = new QListWidget;
  d->ListWidget->setSortingEnabled(false);
  this->connect(d->ListWidget, SIGNAL(itemChanged(QListWidgetItem*)),
                this, SLOT(itemChanged(QListWidgetItem*)));
  groupListLayout->addWidget(d->ListWidget);

  d->ScalarRange = new QLabel("Scalar Range: 0.0 - 1.0", this);
  groupBoxLayout->addWidget(d->ScalarRange);
  d->ScalarRange->setVisible(false);

  QVBoxLayout *buttonBox = new QVBoxLayout;
  QPushButton *addValueButton = new QPushButton("Add Value", this);
  this->connect(addValueButton, SIGNAL(clicked()), this, SLOT(addScalar()));
  buttonBox->addWidget(addValueButton);
  QPushButton *addRangeButton = new QPushButton("Add Range", this);
  this->connect(addRangeButton, SIGNAL(clicked()), this, SLOT(addRange()));
  buttonBox->addWidget(addRangeButton);
  QPushButton *deleteButton = new QPushButton("Delete", this);
  this->connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteScalar()));
  buttonBox->addWidget(deleteButton);
  QPushButton *deleteAllButton = new QPushButton("Delete All", this);
  this->connect(deleteAllButton, SIGNAL(clicked()), this, SLOT(deleteAllScalars()));
  buttonBox->addWidget(deleteAllButton);
  QCheckBox *scientificModeCheckBox = new QCheckBox("Scientific");
  buttonBox->addWidget(scientificModeCheckBox);
  groupListLayout->addLayout(buttonBox);
  groupBoxLayout->addLayout(groupListLayout);

  // create top-level layout for the group box
  this->setLayout(groupBoxLayout);

  this->setScalars(pqSMAdaptor::getMultipleElementProperty(smProperty, pqSMAdaptor::UNCHECKED));
}

pqScalarValueListPropertyWidget::~pqScalarValueListPropertyWidget()
{
  d->VTKRangeConnector->Disconnect();
  delete d;
}

void pqScalarValueListPropertyWidget::setScalars(const QVariantList &values)
{
  d->ListWidget->clear();

  foreach(const QVariant &value, values)
    {
    QListWidgetItem *item = new QListWidgetItem(value.toString());
    item->setFlags(Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable |
                   Qt::ItemIsEditable);
    item->setData(Qt::UserRole, value);
    d->ListWidget->addItem(item);
    }
  emit scalarsChanged();
}

QVariantList pqScalarValueListPropertyWidget::scalars() const
{
  QVariantList values;

  int row = 0;
  while(const QListWidgetItem *item = d->ListWidget->item(row++))
    {
    values.append(item->data(Qt::UserRole));
    }

  return values;
}

void pqScalarValueListPropertyWidget::itemChanged(QListWidgetItem *item)
{
  QVariant value = item->text();

  // check that the new value is a valid number
  bool ok;
  value.toDouble(&ok);

  if(ok)
    {
    // set the value
    item->setData(Qt::UserRole, value);
    emit scalarsChanged();
    }
  else
    {
    // restore previous value
    item->setText(item->data(Qt::UserRole).toString());
    }
}

void pqScalarValueListPropertyWidget::addScalar()
{
  // add new value
  QListWidgetItem *item = addScalar(0.0);

  // set item as current
  d->ListWidget->setCurrentItem(item);

  // open editor for new item
  d->ListWidget->editItem(item);

  emit scalarsChanged();
}

QListWidgetItem* pqScalarValueListPropertyWidget::addScalar(double scalar)
{
  QListWidgetItem *item = new QListWidgetItem;
  item->setFlags(Qt::ItemIsEnabled |
                 Qt::ItemIsSelectable |
                 Qt::ItemIsEditable);
  item->setText(QString::number(scalar));
  item->setData(Qt::UserRole, scalar);
  d->ListWidget->addItem(item);

  emit scalarsChanged();

  return item;
}

void pqScalarValueListPropertyWidget::addRange()
{
  double range_min, range_max;
  if(!this->getRange(range_min, range_max))
    {
    range_min=0.0;
    range_max=10.0;
    }
  pqSampleScalarAddRangeDialog dialog(range_min, range_max, 10, false);

  if(dialog.exec() != QDialog::Accepted)
    {
    return;
    }

  const double from = dialog.from();
  const double to = dialog.to();
  const int steps = dialog.steps();
  const bool logarithmic = dialog.logarithmic();

  if(steps < 2)
    {
    return;
    }

  if(from == to)
    {
    return;
    }

  if(logarithmic)
    {
    const double sign = from < 0 ? -1.0 : 1.0;
    const double log_from = std::log10(std::abs(from ? from : 1.0e-6 * (from - to)));
    const double log_to = std::log10(std::abs(to ? to : 1.0e-6 * (to - from)));

    for(int i = 0; i != steps; i++)
      {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);

      addScalar(sign * pow(10.0, (1.0 - mix) * log_from + (mix) * log_to));
      }
    }
  else
    {
    for(int i = 0; i != steps; i++)
      {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);

      addScalar((1.0 - mix) * from + (mix) * to);
      }
    }
}

void pqScalarValueListPropertyWidget::deleteScalar()
{
  d->ListWidget->takeItem(d->ListWidget->currentRow());

  emit scalarsChanged();
}

void pqScalarValueListPropertyWidget::deleteAllScalars()
{
  d->ListWidget->clear();

  emit scalarsChanged();
}

void pqScalarValueListPropertyWidget::setRangeDomain(
  vtkSMDoubleRangeDomain* smRangeDomain)
{
  if(d->RangeDomain)
    {
    d->VTKRangeConnector->Disconnect();
    }
    
  d->RangeDomain = smRangeDomain;
 
  if(d->RangeDomain)
    {
    d->VTKRangeConnector->Connect(
        d->RangeDomain, vtkCommand::DomainModifiedEvent,
        this, SLOT(smRangeModified()));
    d->ScalarRange->setVisible(true);
    this->smRangeModified();
    }
  else
    {
    d->ScalarRange->setVisible(false);
    }
}

void pqScalarValueListPropertyWidget::smRangeModified()
{
  double range_min;
  double range_max;
  if(this->getRange(range_min, range_max))
    {
    d->ScalarRange->setText(
      tr("Value Range: [%1, %2]").arg(range_min).arg(range_max));
    }
  else
    {
    d->ScalarRange->setText(tr("Value Range: unlimited"));
    }
}

bool pqScalarValueListPropertyWidget::getRange(double& range_min, double& range_max)
{
  // Return the range of values in the input (if available)
  if(d->RangeDomain)
    {
    int min_exists = 0;
    range_min = d->RangeDomain->GetMinimum(0, min_exists);
    
    int max_exists = 0;
    range_max = d->RangeDomain->GetMaximum(0, max_exists);
    
    if(min_exists && max_exists)
      {
      return true;
      }
    }

  return false;
}
