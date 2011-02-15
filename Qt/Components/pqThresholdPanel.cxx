/*=========================================================================

   Program: ParaView
   Module:    pqThresholdPanel.cxx

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

#include "pqThresholdPanel.h"

#include <QComboBox>
#include <QLineEdit>
#include <QSlider>

#include "pqDoubleRangeWidget.h"
#include "pqSMAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "ui_pqThresholdPanel.h"

class pqThresholdPanel::pqUI : public Ui::ThresholdPanel { };

pqThresholdPanel::pqThresholdPanel(pqProxy* pxy, QWidget* p) :
  pqNamedObjectPanel(pxy, p)
{
  this->UI = new pqUI;
  this->UI->setupUi(this);

  this->linkServerManagerProperties();

  QObject::connect(this->UI->ThresholdBetween_0, SIGNAL(valueEdited(double)),
                   this, SLOT(lowerChanged(double)));
  QObject::connect(this->UI->ThresholdBetween_1, SIGNAL(valueEdited(double)),
                   this, SLOT(upperChanged(double)));

  QObject::connect(this->findChild<QComboBox*>("SelectInputScalars"),
    SIGNAL(activated(int)), this, SLOT(variableChanged()),
    Qt::QueuedConnection);

  //setup the proper tab order. This process requires us to find
  //the correct child widgets of the threshold objects. Without these
  //setting the tab breaks down
  QSlider *Slider0 = this->UI->ThresholdBetween_0->findChild<QSlider*>("Slider");
  QLineEdit *LineEdit0 = this->UI->ThresholdBetween_0->findChild<QLineEdit*>("LineEdit");

  QSlider *Slider1 = this->UI->ThresholdBetween_1->findChild<QSlider*>("Slider");
  QLineEdit *LineEdit1 = this->UI->ThresholdBetween_1->findChild<QLineEdit*>("LineEdit");

  //now setup the correct order
  QWidget::setTabOrder(this->UI->SelectInputScalars, Slider0);
  QWidget::setTabOrder(Slider0, LineEdit0);
  QWidget::setTabOrder(LineEdit0, Slider1);
  QWidget::setTabOrder(Slider1, LineEdit1);
  QWidget::setTabOrder(LineEdit1,this->UI->AllScalars);  
}

pqThresholdPanel::~pqThresholdPanel()
{
  delete this->UI;
}

void pqThresholdPanel::lowerChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->UI->ThresholdBetween_1->value() < val)
    {
    this->UI->ThresholdBetween_1->setValue(val);
    }
}

void pqThresholdPanel::upperChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->UI->ThresholdBetween_0->value() > val)
    {
    this->UI->ThresholdBetween_0->setValue(val);
    }
}

void pqThresholdPanel::variableChanged()
{
  // when the user changes the variable, adjust the ranges on the ThresholdBetween
  vtkSMProperty* prop = this->proxy()->GetProperty("ThresholdBetween");
  QList<QVariant> range = pqSMAdaptor::getElementPropertyDomain(prop);
  if(range.size() == 2 && range[0].isValid() && range[1].isValid())
    {
    this->UI->ThresholdBetween_0->setValue(range[0].toDouble());
    this->UI->ThresholdBetween_1->setValue(range[1].toDouble());
    }
}
