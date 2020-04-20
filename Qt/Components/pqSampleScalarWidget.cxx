/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarWidget.cxx

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

#include "pqSampleScalarWidget.h"
#include "pqSampleScalarAddRangeDialog.h"
#include "pqScalarSetModel.h"

#include "ui_pqSampleScalarWidget.h"

#include <QKeyEvent>
#include <vtkMemberFunctionCommand.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMDoubleVectorProperty.h>

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarWidget::pqImplementation

class pqSampleScalarWidget::pqImplementation
{
public:
  pqImplementation()
    : SampleProperty(0)
    , RangeProperty(0)
    , UI(new Ui::pqSampleScalarWidget())
    , IgnorePropertyChange(false)
  {
  }

  ~pqImplementation() { delete this->UI; }

  /// Callback object used to connect property events to member methods
  vtkSmartPointer<vtkCommand> PropertyObserver;
  vtkSmartPointer<vtkCommand> DomainObserver;
  pqSMProxy ControlledProxy;
  vtkSMDoubleVectorProperty* SampleProperty;
  vtkSMProperty* RangeProperty;
  Ui::pqSampleScalarWidget* const UI;
  pqScalarSetModel Model;
  bool IgnorePropertyChange;
};

pqSampleScalarWidget::pqSampleScalarWidget(bool preserveOrder, QWidget* Parent)
  : Superclass(Parent)
  , Implementation(new pqImplementation())
{
  VTK_LEGACY_BODY(pqSampleScalarWidget, "ParaView 5.8");

  this->Implementation->PropertyObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqSampleScalarWidget::onControlledPropertyChanged));

  this->Implementation->DomainObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqSampleScalarWidget::onControlledPropertyDomainChanged));

  this->Implementation->UI->setupUi(this);

  this->Implementation->Model.setPreserveOrder(preserveOrder);
  this->Implementation->UI->Values->setModel(&this->Implementation->Model);
  this->Implementation->UI->Values->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Implementation->UI->Values->setSelectionMode(QAbstractItemView::ExtendedSelection);

  this->Implementation->UI->Delete->setEnabled(false);
  this->Implementation->UI->Values->installEventFilter(this);

  connect(&this->Implementation->Model, SIGNAL(layoutChanged()), this, SIGNAL(samplesChanged()));

  connect(&this->Implementation->Model, SIGNAL(layoutChanged()), this, SLOT(onSamplesChanged()));

  connect(this->Implementation->UI->Values->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(this->Implementation->UI->Delete, SIGNAL(clicked()), this, SLOT(onDelete()));
  connect(this->Implementation->UI->DeleteAll, SIGNAL(clicked()), this, SLOT(onDeleteAll()));

  connect(this->Implementation->UI->NewValue, SIGNAL(clicked()), this, SLOT(onNewValue()));

  connect(this->Implementation->UI->NewRange, SIGNAL(clicked()), this, SLOT(onNewRange()));

  connect(this->Implementation->UI->ScientificNotation, SIGNAL(toggled(bool)), this,
    SLOT(onScientificNotation(bool)));

  this->onSamplesChanged();
}

pqSampleScalarWidget::~pqSampleScalarWidget()
{
  if (this->Implementation->RangeProperty)
  {
    this->Implementation->RangeProperty->RemoveObserver(this->Implementation->DomainObserver);
  }

  if (this->Implementation->SampleProperty)
  {
    auto domain = this->Implementation->SampleProperty->FindDomain<vtkSMDoubleRangeDomain>();
    if (domain)
    {
      domain->RemoveObserver(this->Implementation->PropertyObserver);
    }
  }

  delete this->Implementation;
}

void pqSampleScalarWidget::setDataSources(pqSMProxy controlled_proxy,
  vtkSMDoubleVectorProperty* sample_property, vtkSMProperty* range_property)
{
  if (this->Implementation->SampleProperty)
  {
    this->Implementation->SampleProperty->RemoveObserver(this->Implementation->PropertyObserver);
  }
  if (this->Implementation->RangeProperty)
  {
    this->Implementation->RangeProperty->RemoveObserver(this->Implementation->DomainObserver);
  }

  this->Implementation->ControlledProxy = controlled_proxy;
  this->Implementation->SampleProperty = sample_property;
  this->Implementation->RangeProperty = range_property;

  if (this->Implementation->SampleProperty)
  {
    auto domain = this->Implementation->SampleProperty->FindDomain<vtkSMDoubleRangeDomain>();
    if (domain)
    {
      domain->AddObserver(vtkCommand::DomainModifiedEvent, this->Implementation->PropertyObserver);
    }
  }

  if (this->Implementation->RangeProperty)
  {
    this->Implementation->RangeProperty->AddObserver(
      vtkCommand::DomainModifiedEvent, this->Implementation->DomainObserver);
  }

  this->reset();
  this->onSamplesChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSampleScalarWidget::samples()
{
  QList<QVariant> list;
  if (this->Implementation->SampleProperty)
  {
    const QList<double> sample_list = this->Implementation->Model.values();
    foreach (double v, sample_list)
    {
      list.push_back(QVariant(v));
    }
  }
  return list;
}

//-----------------------------------------------------------------------------
void pqSampleScalarWidget::setSamples(QList<QVariant> list)
{
  this->Implementation->Model.clear();
  foreach (QVariant v, list)
  {
    if (v.canConvert(QVariant::Double))
    {
      this->Implementation->Model.insert(v.toDouble());
    }
  }
}

//-----------------------------------------------------------------------------
void pqSampleScalarWidget::accept()
{
  this->Implementation->IgnorePropertyChange = true;

  if (this->Implementation->SampleProperty)
  {
    const QList<double> sample_list = this->Implementation->Model.values();

    this->Implementation->SampleProperty->SetNumberOfElements(sample_list.size());
    for (int i = 0; i != sample_list.size(); ++i)
    {
      this->Implementation->SampleProperty->SetElement(i, sample_list[i]);
    }
  }

  if (this->Implementation->ControlledProxy)
  {
    this->Implementation->ControlledProxy->UpdateVTKObjects();
  }

  this->Implementation->IgnorePropertyChange = false;
  this->onSamplesChanged();
}

void pqSampleScalarWidget::reset()
{
  this->onControlledPropertyDomainChanged();

  // Set the list of values
  QList<double> values;

  if (this->Implementation->SampleProperty)
  {
    const int value_count = this->Implementation->SampleProperty->GetNumberOfElements();
    for (int i = 0; i != value_count; ++i)
    {
      values.push_back(this->Implementation->SampleProperty->GetElement(i));
    }
  }

  this->Implementation->Model.clear();
  for (int i = 0; i != values.size(); ++i)
  {
    this->Implementation->Model.insert(values[i]);
  }
}

void pqSampleScalarWidget::onSamplesChanged()
{
  this->Implementation->UI->DeleteAll->setEnabled(this->Implementation->Model.values().size());
}

void pqSampleScalarWidget::onSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  this->Implementation->UI->Delete->setEnabled(
    this->Implementation->UI->Values->selectionModel()->selectedIndexes().size());
}

void pqSampleScalarWidget::onDelete()
{
  QList<int> rows;
  for (int i = 0; i != this->Implementation->Model.rowCount(); ++i)
  {
    if (this->Implementation->UI->Values->selectionModel()->isRowSelected(i, QModelIndex()))
      rows.push_back(i);
  }

  for (int i = rows.size() - 1; i >= 0; --i)
  {
    this->Implementation->Model.erase(rows[i]);
  }

  this->Implementation->UI->Values->selectionModel()->clear();

  this->onSamplesChanged();
  Q_EMIT samplesChanged();
}
void pqSampleScalarWidget::onDeleteAll()
{
  this->Implementation->Model.clear();

  this->Implementation->UI->Values->selectionModel()->clear();

  this->onSamplesChanged();
  Q_EMIT samplesChanged();
}
void pqSampleScalarWidget::onNewValue()
{
  double new_value = 0.0;
  QList<double> values = this->Implementation->Model.values();
  if (values.size())
  {
    double delta = 0.1;
    if (values.size() > 1)
    {
      delta = values[values.size() - 1] - values[values.size() - 2];
    }
    new_value = values[values.size() - 1] + delta;
  }

  QModelIndex idx = this->Implementation->Model.insert(new_value);

  this->Implementation->UI->Values->setCurrentIndex(idx);
  this->Implementation->UI->Values->edit(idx);
  this->onSamplesChanged();
}

void pqSampleScalarWidget::onNewRange()
{
  double current_min = 0.0;
  double current_max = 1.0;
  this->getRange(current_min, current_max);

  pqSampleScalarAddRangeDialog dialog(current_min, current_max, 10, false);
  if (QDialog::Accepted != dialog.exec())
  {
    return;
  }

  const double from = dialog.from();
  const double to = dialog.to();
  const unsigned long steps = dialog.steps();
  const bool logarithmic = dialog.logarithmic();

  if (steps < 2)
    return;

  if (from == to)
    return;

  if (logarithmic)
  {
    const double sign = from < 0 ? -1.0 : 1.0;
    const double log_from = log10(fabs(from ? from : 1.0e-6 * (from - to)));
    const double log_to = log10(fabs(to ? to : 1.0e-6 * (to - from)));

    for (unsigned long i = 0; i != steps; ++i)
    {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);
      this->Implementation->Model.insert(sign * pow(10.0, (1.0 - mix) * log_from + (mix)*log_to));
    }
  }
  else
  {
    for (unsigned long i = 0; i != steps; ++i)
    {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);
      this->Implementation->Model.insert((1.0 - mix) * from + (mix)*to);
    }
  }

  this->onSamplesChanged();
  Q_EMIT samplesChanged();
}

void pqSampleScalarWidget::onSelectAll()
{
  for (int i = 0; i != this->Implementation->Model.rowCount(); ++i)
  {
    this->Implementation->UI->Values->selectionModel()->select(
      this->Implementation->Model.index(i, 0), QItemSelectionModel::Select);
  }
}

void pqSampleScalarWidget::onScientificNotation(bool enabled)
{
  if (enabled)
  {
    this->Implementation->Model.setFormat('e');
  }
  else
  {
    this->Implementation->Model.setFormat('g');
  }
}

void pqSampleScalarWidget::onControlledPropertyChanged()
{
  if (this->Implementation->IgnorePropertyChange)
  {
    return;
  }
  this->onControlledPropertyDomainChanged();
}

void pqSampleScalarWidget::onControlledPropertyDomainChanged()
{
  double range_min;
  double range_max;
  if (this->getRange(range_min, range_max))
  {
    this->Implementation->UI->ScalarRange->setText(
      tr("Value Range: [%1, %2]").arg(range_min).arg(range_max));
  }
  else
  {
    this->Implementation->UI->ScalarRange->setText(tr("Value Range: unlimited"));
  }
  this->onSamplesChanged();
}

bool pqSampleScalarWidget::getRange(double& range_min, double& range_max)
{
  // Return the range of values in the input (if available)
  if (this->Implementation->SampleProperty)
  {
    auto domain = this->Implementation->SampleProperty->FindDomain<vtkSMDoubleRangeDomain>();
    if (domain)
    {
      int min_exists = 0;
      range_min = domain->GetMinimum(0, min_exists);

      int max_exists = 0;
      range_max = domain->GetMaximum(0, max_exists);

      if (min_exists && max_exists)
      {
        return true;
      }
    }
  }

  return false;
}

bool pqSampleScalarWidget::eventFilter(QObject* object, QEvent* e)
{
  if (object == this->Implementation->UI->Values && e->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
    if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
    {
      this->onDelete();
    }
  }

  return QWidget::eventFilter(object, e);
}
