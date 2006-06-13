/*=========================================================================

   Program:   ParaQ
   Module:    pqSampleScalarWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "ui_pqSampleScalarWidget.h"

pqSampleScalarWidget::pqSampleScalarWidget(QWidget* Parent) :
  base(Parent),
  Implementation(new Ui::pqSampleScalarWidget())
{
  this->Implementation->setupUi(this);
  
  this->Implementation->RangeMin->setValidator(
    new QDoubleValidator(this->Implementation->RangeMin));
    
  this->Implementation->RangeMax->setValidator(
    new QDoubleValidator(this->Implementation->RangeMax));
  
  this->Implementation->RangeCount->setValidator(
    new QIntValidator(2, 9999, this->Implementation->RangeCount));
  
  this->Implementation->Value->setValidator(
    new QDoubleValidator(this->Implementation->Value));
  
  connect(this->Implementation->AddRange, SIGNAL(clicked()), this, SLOT(onAddRange()));
  connect(this->Implementation->AddValue, SIGNAL(clicked()), this, SLOT(onAddValue()));
  connect(this->Implementation->DeleteAll, SIGNAL(clicked()), this, SLOT(onDeleteAll()));
  connect(this->Implementation->DeleteSelected, SIGNAL(clicked()), this, SLOT(onDeleteSelected()));
}

pqSampleScalarWidget::~pqSampleScalarWidget()
{
  delete this->Implementation;
}

void pqSampleScalarWidget::setSamples(const QList<double>& samples)
{
  this->Implementation->Values->clear();
  
  for(int i = 0; i != samples.size(); ++i)
    {
    this->Implementation->Values->addItem(QString::number(samples[i]));
    }
   
  this->Implementation->Values->sortItems();
  emit samplesChanged();
}

const QList<double> pqSampleScalarWidget::getSamples()
{
  QList<double> samples;
  
  for(int i = 0; i != this->Implementation->Values->count(); ++i)
    {
    samples.push_back(this->Implementation->Values->item(i)->text().toDouble());
    }
    
  return samples;
}

void pqSampleScalarWidget::onAddRange()
{
  const double range_min = this->Implementation->RangeMin->text().toDouble();
  const double range_max = this->Implementation->RangeMax->text().toDouble();
  const int range_count = this->Implementation->RangeCount->text().toInt();

  if(range_count < 2)
    return;
    
  if(range_min == range_max)
    return;

  for(int i = 0; i != range_count; ++i)
    {
    const double mix = static_cast<double>(i) / static_cast<double>(range_count - 1);
    
    this->Implementation->Values->addItem(QString::number(
      (1.0 - mix) * range_min + (mix) * range_max));
    }
  
  this->Implementation->Values->sortItems();
  emit samplesChanged();
}

void pqSampleScalarWidget::onAddValue()
{
  this->Implementation->Values->addItem(
    QString::number(this->Implementation->Value->text().toDouble()));

  this->Implementation->Values->sortItems();
  emit samplesChanged();
}

void pqSampleScalarWidget::onDeleteAll()
{
  this->Implementation->Values->clear();
  emit samplesChanged();
}

void pqSampleScalarWidget::onDeleteSelected()
{
  for(int i = this->Implementation->Values->count() - 1; i >= 0; --i)
    {
    if(this->Implementation->Values->isItemSelected(this->Implementation->Values->item(i)))
      {
      this->Implementation->Values->model()->removeRow(i);
      }
    }

  this->Implementation->Values->sortItems();
  emit samplesChanged();
}
