/*=========================================================================

   Program:   ParaQ
   Module:    pqThresholdPanel.cxx

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

#include "pqThresholdPanel.h"

// Qt includes
#include <QDoubleSpinBox>
#include <QSlider>
#include <QComboBox>

// VTK includes

// ParaView includes
#include "vtkSMStringVectorProperty.h"
#include "vtkSMEnumerationDomain.h"

// ParaQ includes
#include "pqSMAdaptor.h"
#include "pqPropertyManager.h"
#include "pqDoubleSpinBoxDomain.h"
#include "pqComboBoxDomain.h"


pqThresholdPanel::pqThresholdPanel(QWidget* p) :
  pqLoadedFormObjectPanel(":/pqWidgets/ThresholdPanel.ui", p)
{
  this->AttributeMode = this->findChild<QComboBox*>("AttributeMode");
  
  QObject::connect(this->AttributeMode, SIGNAL(activated(int)),
                   this, SLOT(attributeModeChanged(int)));

  this->LowerSlider = this->findChild<QSlider*>("LowerThresholdSlider");
  this->UpperSlider = this->findChild<QSlider*>("UpperThresholdSlider");
  this->LowerSpin = this->findChild<QDoubleSpinBox*>("ThresholdBetweenSpin:0");
  this->UpperSpin = this->findChild<QDoubleSpinBox*>("ThresholdBetweenSpin:1");

  QObject::connect(this->LowerSlider, SIGNAL(valueChanged(int)),
                   this, SLOT(lowerSliderChanged()));
  QObject::connect(this->UpperSlider, SIGNAL(valueChanged(int)),
                   this, SLOT(upperSliderChanged()));
  QObject::connect(this->LowerSpin, SIGNAL(valueChanged(double)),
                   this, SLOT(lowerSpinChanged()));
  QObject::connect(this->UpperSpin, SIGNAL(valueChanged(double)),
                   this, SLOT(upperSpinChanged()));

}

pqThresholdPanel::~pqThresholdPanel()
{
}

void pqThresholdPanel::accept()
{
  // accept widgets controlled by the parent class
  pqLoadedFormObjectPanel::accept();

  vtkSMStringVectorProperty* Property;
  Property = vtkSMStringVectorProperty::SafeDownCast(
       this->proxy()->GetProperty("SelectInputScalars"));
 
  int idx = this->AttributeMode->currentIndex(); 
  QString mode = this->AttributeMode->itemData(idx).toString();

  Property->SetElement(3, mode.toAscii().data());

  this->proxy()->UpdateVTKObjects();
}

void pqThresholdPanel::reset()
{
  // reset widgets controlled by the parent class
  pqLoadedFormObjectPanel::reset();

  vtkSMStringVectorProperty* Property;
  Property = vtkSMStringVectorProperty::SafeDownCast(
       this->proxy()->GetProperty("SelectInputScalars"));
 
  QString mode = Property->GetElement(3);
  
  int idx = this->AttributeMode->findData(mode);
  if(idx != -1)
    {
    this->AttributeMode->setCurrentIndex(idx);
    }
}

void pqThresholdPanel::linkServerManagerProperties()
{
  // parent class hooks up some of our widgets in the ui
  pqLoadedFormObjectPanel::linkServerManagerProperties();
  
  
  // set up the attribute mode combo box
  vtkSMStringVectorProperty* AttributeProperty;
  AttributeProperty = vtkSMStringVectorProperty::SafeDownCast(
       this->proxy()->GetProperty("SelectInputScalars"));
  AttributeProperty->UpdateDependentDomains();
  
  vtkSMEnumerationDomain* domain;
  domain = vtkSMEnumerationDomain::SafeDownCast(
        AttributeProperty->GetDomain("field_list"));
  unsigned numEntries = domain->GetNumberOfEntries();
  int AttributeIndex = 0;
  int AttributeModeVal = atoi(AttributeProperty->GetElement(3));
  for(unsigned int i=0; i<numEntries; i++)
    {
    this->AttributeMode->addItem(domain->GetEntryText(i),
                                 domain->GetEntryValue(i));
    if(domain->GetEntryValue(i)==AttributeModeVal)
      {
      AttributeIndex = i;
      }
    }
  this->AttributeMode->setCurrentIndex(AttributeIndex);

  
  // connect domain to scalar combo box
  vtkSMProperty* Property = this->proxy()->GetProperty("SelectInputScalars");
  QComboBox* Scalars = this->findChild<QComboBox*>("SelectInputScalars");
  pqComboBoxDomain* d0 = new pqComboBoxDomain(Scalars, Property);
  d0->setObjectName("ScalarsDomain");

  // connect domain to spin boxes
  Property = this->proxy()->GetProperty("ThresholdBetween");
  Property->UpdateDependentDomains();
  pqDoubleSpinBoxDomain* d1 = new pqDoubleSpinBoxDomain(this->LowerSpin,
                            Property,
                            0);
  d1->setObjectName("lowerSpinDomain");
  pqDoubleSpinBoxDomain* d2 = new pqDoubleSpinBoxDomain(this->UpperSpin,
                            Property,
                            1);
  d2->setObjectName("upperSpinDomain");
}

void pqThresholdPanel::unlinkServerManagerProperties()
{
  this->AttributeMode->clear();
  
  QComboBox* Scalars = this->findChild<QComboBox*>("SelectInputScalars");
  pqComboBoxDomain* d0 = Scalars->findChild<pqComboBoxDomain*>("ScalarsDomain");
  delete d0;

  pqDoubleSpinBoxDomain* d1;
  d1 = this->LowerSpin->findChild<pqDoubleSpinBoxDomain*>("lowerSpinDomain");
  delete d1;
  
  pqDoubleSpinBoxDomain* d2;
  d2 = this->UpperSpin->findChild<pqDoubleSpinBoxDomain*>("upperSpinDomain");
  delete d2;
  
  // parent class un-hooks some of our widgets in the ui
  pqLoadedFormObjectPanel::unlinkServerManagerProperties();
}


static bool IsSetting = false;

void pqThresholdPanel::lowerSpinChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    // spin changed, so update the slider
    // note: the slider is just a tag-along-guy to give a general indication of
    // range
    double v = this->LowerSpin->value();
    double range = this->LowerSpin->maximum() - this->LowerSpin->minimum();
    double fraction = (v - this->LowerSpin->minimum()) / range;
    int sliderVal = qRound(fraction * 100.0);  // slider range 0-100
    this->LowerSlider->setValue(sliderVal);
    IsSetting = false;
    }
}

void pqThresholdPanel::upperSpinChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    // spin changed, so update the slider
    // note: the slider is just a tag-along-guy to give a general indication of
    // range
    double v = this->UpperSpin->value();
    double range = this->UpperSpin->maximum() - this->UpperSpin->minimum();
    double fraction = (v - this->UpperSpin->minimum()) / range;
    int sliderVal = qRound(fraction * 100.0);  // slider range 0-100
    this->UpperSlider->setValue(sliderVal);
    IsSetting = false;
    }
}

void pqThresholdPanel::lowerSliderChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    double fraction = this->LowerSlider->value() * 100.0;
    double range = this->LowerSpin->maximum() - this->LowerSpin->minimum();
    double v = (fraction * range) + this->LowerSpin->minimum();
    this->LowerSpin->setValue(v);
    IsSetting = false;
    }
}

void pqThresholdPanel::upperSliderChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    double fraction = this->UpperSlider->value() * 100.0;
    double range = this->UpperSpin->maximum() - this->UpperSpin->minimum();
    double v = (fraction * range) + this->UpperSpin->minimum();
    this->UpperSpin->setValue(v);
    IsSetting = false;
    }
}

void pqThresholdPanel::attributeModeChanged(int idx)
{
  vtkSMStringVectorProperty* Property;
  Property = vtkSMStringVectorProperty::SafeDownCast(
       this->proxy()->GetProperty("SelectInputScalars"));

  QString mode = this->AttributeMode->itemData(idx).toString();

  Property->SetUncheckedElement(3, mode.toAscii().data());
  Property->UpdateDependentDomains();

  emit this->canAcceptOrReject(true);
}

