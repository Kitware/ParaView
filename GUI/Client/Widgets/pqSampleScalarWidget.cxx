/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarWidget.cxx

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

#include "pqSampleScalarWidget.h"
#include "pqScalarSetModel.h"

#include "ui_pqSampleScalarAddRangeDialog.h"
#include "ui_pqSampleScalarWidget.h"

#include <vtkMemberFunctionCommand.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMDoubleVectorProperty.h>

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarWidget::pqAddRangeDialog

class pqSampleScalarWidget::pqAddRangeDialog :
  public QDialog
{
public:
  pqAddRangeDialog()
  {
    this->UI.setupUi(this);

    this->UI.From->setValidator(
      new QDoubleValidator(this->UI.From));
      
    this->UI.To->setValidator(
      new QDoubleValidator(this->UI.To));
    
    this->UI.Steps->setValidator(
      new QIntValidator(2, 9999, this->UI.Steps));
  }
  
  Ui::pqSampleScalarAddRangeDialog UI;
};

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarWidget::pqImplementation

class pqSampleScalarWidget::pqImplementation
{
public:
  pqImplementation() :
    SampleProperty(0),
    RangeProperty(0),
    UI(new Ui::pqSampleScalarWidget()),
    IgnorePropertyChange(false)
  {
  }
  
  ~pqImplementation()
  {
    delete this->UI;
  }
  
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

pqSampleScalarWidget::pqSampleScalarWidget(QWidget* Parent) :
  base(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->PropertyObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqSampleScalarWidget::onControlledPropertyChanged));

  this->Implementation->DomainObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqSampleScalarWidget::onControlledPropertyDomainChanged));

  this->Implementation->UI->setupUi(this);

  this->Implementation->UI->Values->setModel(&this->Implementation->Model);
  this->Implementation->UI->Values->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Implementation->UI->Values->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->Implementation->UI->Delete->setEnabled(false);
  this->Implementation->UI->SelectAll->setEnabled(false);
  
  connect(
    &this->Implementation->Model,
    SIGNAL(layoutChanged()),
    this,
    SIGNAL(samplesChanged()));
  
  connect(
    &this->Implementation->Model,
    SIGNAL(layoutChanged()),
    this,
    SLOT(onSamplesChanged()));
    
  connect(
    this->Implementation->UI->Values->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    this,
    SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(
    this->Implementation->UI->Delete,
    SIGNAL(clicked()),
    this,
    SLOT(onDelete()));
    
  connect(
    this->Implementation->UI->NewValue,
    SIGNAL(clicked()),
    this,
    SLOT(onNewValue()));
    
  connect(
    this->Implementation->UI->NewRange,
    SIGNAL(clicked()),
    this,
    SLOT(onNewRange()));
    
  connect(
    this->Implementation->UI->SelectAll,
    SIGNAL(clicked()),
    this,
    SLOT(onSelectAll()));
    
  connect(
    this->Implementation->UI->ScientificNotation,
    SIGNAL(toggled(bool)),
    this,
    SLOT(onScientificNotation(bool)));
}

pqSampleScalarWidget::~pqSampleScalarWidget()
{
  if(this->Implementation->SampleProperty)
    {
    this->Implementation->SampleProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }

  delete this->Implementation;
}

void pqSampleScalarWidget::setDataSources(
  pqSMProxy controlled_proxy,
  vtkSMDoubleVectorProperty* sample_property,
  vtkSMProperty* range_property)
{
  if(this->Implementation->SampleProperty)
    {
    this->Implementation->SampleProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }
  if(this->Implementation->RangeProperty)
    {
    this->Implementation->RangeProperty->RemoveObserver(
      this->Implementation->DomainObserver);
    }
    
  this->Implementation->ControlledProxy = controlled_proxy;
  this->Implementation->SampleProperty = sample_property;
  this->Implementation->RangeProperty = range_property;

  if(this->Implementation->SampleProperty)
    {
    this->Implementation->SampleProperty->AddObserver(
      vtkCommand::ModifiedEvent,
      this->Implementation->PropertyObserver);
    }
  
  if(this->Implementation->RangeProperty)
    {
    this->Implementation->RangeProperty->AddObserver(
      vtkCommand::ModifiedEvent,
      this->Implementation->DomainObserver);
    }
    
  this->reset();
}

void pqSampleScalarWidget::accept()
{
  this->Implementation->IgnorePropertyChange = true;
  
  if(this->Implementation->SampleProperty)
    {
    const QList<double> samples = this->Implementation->Model.values();
    
    this->Implementation->SampleProperty->SetNumberOfElements(samples.size());
    for(int i = 0; i != samples.size(); ++i)
      {
      this->Implementation->SampleProperty->SetElement(i, samples[i]);
      }
    }

  if(this->Implementation->ControlledProxy)
    {
    this->Implementation->ControlledProxy->UpdateVTKObjects();
    }
    
  this->Implementation->IgnorePropertyChange = false;
}

void pqSampleScalarWidget::reset()
{
  this->onControlledPropertyDomainChanged();

  // Set the list of values
  QList<double> values;
  
  if(this->Implementation->SampleProperty)
    {
    const int value_count = this->Implementation->SampleProperty->GetNumberOfElements();
    for(int i = 0; i != value_count; ++i)
      {
      values.push_back(this->Implementation->SampleProperty->GetElement(i));
      }
    }

  this->Implementation->Model.clear();
  for(int i = 0; i != values.size(); ++i)
    {
    this->Implementation->Model.insert(values[i]);
    }
}

void pqSampleScalarWidget::onSamplesChanged()
{
  this->Implementation->UI->SelectAll->setEnabled(
    this->Implementation->Model.rowCount());
}

void pqSampleScalarWidget::onSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  this->Implementation->UI->Delete->setEnabled(
    this->Implementation->UI->Values->selectionModel()->selectedIndexes().size());
}

void pqSampleScalarWidget::onDelete()
{
  QList<int> rows;
  for(int i = 0; i != this->Implementation->Model.rowCount(); ++i)
    {
    if(this->Implementation->UI->Values->selectionModel()->isRowSelected(i, QModelIndex()))
      rows.push_back(i);
    }

  for(int i = rows.size() - 1; i >= 0; --i)
    {
    this->Implementation->Model.erase(rows[i]);
    }

  this->Implementation->UI->Values->selectionModel()->clear();
  
  emit samplesChanged();
}

void pqSampleScalarWidget::onNewValue()
{
  double new_value = 0.0;
  QList<double> values = this->Implementation->Model.values();
  if(values.size())
    {
    double delta = 0.1;
    if(values.size() > 1)
      {
      delta = values[values.size() - 1] - values[values.size() - 2];
      }
    new_value = values[values.size() - 1] + delta;
    }
    
  this->Implementation->Model.insert(new_value);
}

void pqSampleScalarWidget::onNewRange()
{
  pqAddRangeDialog dialog;
  if(QDialog::Accepted != dialog.exec())
    {
    return;
    }
    
  const double range_min = dialog.UI.From->text().toDouble();
  const double range_max = dialog.UI.To->text().toDouble();
  const int range_count = dialog.UI.Steps->text().toInt();

  if(range_count < 2)
    return;
    
  if(range_min == range_max)
    return;

  for(int i = 0; i != range_count; ++i)
    {
    const double mix = static_cast<double>(i) / static_cast<double>(range_count - 1);
    this->Implementation->Model.insert((1.0 - mix) * range_min + (mix) * range_max);
    }
  
  emit samplesChanged();
}

void pqSampleScalarWidget::onSelectAll()
{
  for(int i = 0; i != this->Implementation->Model.rowCount(); ++i)
    {
    this->Implementation->UI->Values->selectionModel()->select(
      this->Implementation->Model.index(i, 0),
      QItemSelectionModel::Select);
    }
}

void pqSampleScalarWidget::onScientificNotation(bool enabled)
{
  if(enabled)
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
  if(this->Implementation->IgnorePropertyChange)
    {
    return;
    }
    
  this->reset();
}

void pqSampleScalarWidget::onControlledPropertyDomainChanged()
{
  // Display the range of values in the input (if any)
  if(this->Implementation->SampleProperty)
    {
    if(vtkSMDoubleRangeDomain* const domain =
      vtkSMDoubleRangeDomain::SafeDownCast(
        this->Implementation->SampleProperty->GetDomain("scalar_range")))
      {
      int min_exists = 0;
      const double min_value = domain->GetMinimum(0, min_exists);
      
      int max_exists = 0;
      const double max_value = domain->GetMaximum(0, max_exists);
      
      if(min_exists && max_exists)
        {
        this->Implementation->UI->ScalarRange->setText(
          tr("Scalar Range: [%1, %2]").arg(min_value).arg(max_value));
        }
      else
        {
        this->Implementation->UI->ScalarRange->setText(
          tr("Scalar Range: unlimited"));
        }
      }
    }
}

