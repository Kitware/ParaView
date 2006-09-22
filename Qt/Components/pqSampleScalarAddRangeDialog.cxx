/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarAddRangeDialog.cxx

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

#include "pqSampleScalarAddRangeDialog.h"
#include "ui_pqSampleScalarAddRangeDialog.h"

#include <QDoubleValidator>

#include <vtkstd/algorithm>

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarAddRangeDialog::pqImplementation

class pqSampleScalarAddRangeDialog::pqImplementation
{
public:
  Ui::pqSampleScalarAddRangeDialog Ui;
};

///////////////////////////////////////////////////////////////////////////
// pqSampleScalarAddRangeDialog

pqSampleScalarAddRangeDialog::pqSampleScalarAddRangeDialog(
    double default_from,
    double default_to,
    unsigned long default_steps,
    bool default_logarithmic,
    QWidget* Parent) :
  Superclass(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->Ui.setupUi(this);
  
  this->Implementation->Ui.from->setValidator(
    new QDoubleValidator(this->Implementation->Ui.from));
  this->Implementation->Ui.from->setText(QString::number(default_from));
    
  this->Implementation->Ui.to->setValidator(
    new QDoubleValidator(this->Implementation->Ui.to));
  this->Implementation->Ui.to->setText(QString::number(default_to));
  
  this->Implementation->Ui.steps->setValidator(
    new QIntValidator(2, 9999, this->Implementation->Ui.steps));
  this->Implementation->Ui.steps->setText(QString::number(default_steps));
  
  this->Implementation->Ui.log->setChecked(default_logarithmic);
  
  QObject::connect(
    this->Implementation->Ui.from,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(onRangeChanged()));
  
  QObject::connect(
    this->Implementation->Ui.to,
    SIGNAL(textChanged(const QString&)),
    this,
    SLOT(onRangeChanged()));
  
  this->onRangeChanged();
}

pqSampleScalarAddRangeDialog::~pqSampleScalarAddRangeDialog()
{
  delete this->Implementation;
}

const double pqSampleScalarAddRangeDialog::from() const
{
  return this->Implementation->Ui.from->text().toDouble();
}

const double pqSampleScalarAddRangeDialog::to() const
{
  return this->Implementation->Ui.to->text().toDouble();
}

const unsigned long pqSampleScalarAddRangeDialog::steps() const
{
  return this->Implementation->Ui.steps->text().toInt();
}

const bool pqSampleScalarAddRangeDialog::logarithmic() const
{
  return this->Implementation->Ui.log->isChecked();
}

void pqSampleScalarAddRangeDialog::onRangeChanged()
{
  double from_value = this->from();
  double to_value = this->to();
  if(to_value < from_value)
    vtkstd::swap(from_value, to_value);
  const bool signs_differ = from_value < 0 && to_value > 0;
  
  if(signs_differ)
    this->Implementation->Ui.log->setChecked(false);
    
  this->Implementation->Ui.log->setVisible(!signs_differ);
  this->Implementation->Ui.logWarning->setVisible(signs_differ);
}
