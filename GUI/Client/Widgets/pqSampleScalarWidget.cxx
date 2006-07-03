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

#include "ui_pqSampleScalarWidget.h"

#include <vtkMemberFunctionCommand.h>
#include <vtkSMDoubleVectorProperty.h>

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarWidget::pqImplementation

class pqSampleScalarWidget::pqImplementation
{
public:
  pqImplementation() :
    SampleProperty(0),
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
  pqSMProxy ControlledProxy;
  vtkSMDoubleVectorProperty* SampleProperty;
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

  this->Implementation->UI->setupUi(this);
  
  this->Implementation->UI->RangeMin->setValidator(
    new QDoubleValidator(this->Implementation->UI->RangeMin));
    
  this->Implementation->UI->RangeMax->setValidator(
    new QDoubleValidator(this->Implementation->UI->RangeMax));
  
  this->Implementation->UI->RangeCount->setValidator(
    new QIntValidator(2, 9999, this->Implementation->UI->RangeCount));
  
  this->Implementation->UI->Value->setValidator(
    new QDoubleValidator(this->Implementation->UI->Value));
  
  this->Implementation->UI->Values->setModel(&this->Implementation->Model);
  this->Implementation->UI->Values->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Implementation->UI->Values->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->Implementation->UI->DeleteAll->setEnabled(false);
  this->Implementation->UI->DeleteSelected->setEnabled(false);
  
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
  
  connect(this->Implementation->UI->AddRange, SIGNAL(clicked()), this, SLOT(onAddRange()));
  connect(this->Implementation->UI->AddValue, SIGNAL(clicked()), this, SLOT(onAddValue()));
  connect(this->Implementation->UI->DeleteAll, SIGNAL(clicked()), this, SLOT(onDeleteAll()));
  connect(this->Implementation->UI->DeleteSelected, SIGNAL(clicked()), this, SLOT(onDeleteSelected()));
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
  vtkSMDoubleVectorProperty* sample_property)
{
  if(this->Implementation->SampleProperty)
    {
    this->Implementation->SampleProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }
    
  this->Implementation->ControlledProxy = controlled_proxy;
  this->Implementation->SampleProperty = sample_property;

  if(this->Implementation->SampleProperty)
    {
    this->Implementation->SampleProperty->AddObserver(
      vtkCommand::ModifiedEvent,
      this->Implementation->PropertyObserver);
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
  this->Implementation->UI->DeleteAll->setEnabled(
    this->Implementation->Model.rowCount());
}

void pqSampleScalarWidget::onSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  this->Implementation->UI->DeleteSelected->setEnabled(
    this->Implementation->UI->Values->selectionModel()->selectedIndexes().size());
}

void pqSampleScalarWidget::onAddRange()
{
  const double range_min = this->Implementation->UI->RangeMin->text().toDouble();
  const double range_max = this->Implementation->UI->RangeMax->text().toDouble();
  const int range_count = this->Implementation->UI->RangeCount->text().toInt();

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

void pqSampleScalarWidget::onAddValue()
{
  this->Implementation->Model.insert(
    this->Implementation->UI->Value->text().toDouble());
    
  emit samplesChanged();
}

void pqSampleScalarWidget::onDeleteAll()
{
  this->Implementation->Model.clear();
  this->Implementation->UI->Values->selectionModel()->clear();
  
  emit samplesChanged();
}

void pqSampleScalarWidget::onDeleteSelected()
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

void pqSampleScalarWidget::onControlledPropertyChanged()
{
  if(this->Implementation->IgnorePropertyChange)
    {
    return;
    }
    
  // Synchronize Qt widgets with the controlled properties
  this->reset();
}
