/*=========================================================================

   Program:   ParaQ
   Module:    pqPointSourceWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2. 

   See License_v1.2.txt for the full ParaQ license.
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

#include "pqHandleWidget.h"
#include "pqNamedWidgets.h"
#include "pqPointSourceWidget.h"
#include "pqPropertyLinks.h"

#include "ui_pqPointSourceControls.h"

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMNewWidgetRepresentationProxy.h>

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////
// pqPointSourceWidget::pqImplementation

class pqPointSourceWidget::pqImplementation
{
public:
  pqImplementation()
  {
  this->Links.setUseUncheckedProperties(false);
  this->Links.setAutoUpdateVTKObjects(true);
  }
  QWidget ControlsContainer;
  Ui::pqPointSourceControls Controls;

  pqPropertyLinks Links;
};

/////////////////////////////////////////////////////////////////////////
// pqPointSourceWidget

pqPointSourceWidget::pqPointSourceWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p) :
  Superclass(o, pxy, p),
  Implementation(new pqImplementation())
{
  this->Implementation->Controls.setupUi(
    &this->Implementation->ControlsContainer);
    
  this->Implementation->Controls.Radius->
    setValidator(new QDoubleValidator(this->Implementation->Controls.Radius));

  this->layout()->addWidget(&this->Implementation->ControlsContainer);
  QLabel* label =new QLabel("<b>Note: Move mouse and use 'P' key to change point position</b>", this);
  label->setWordWrap(1);
  this->layout()->addWidget(label);

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));
}

//-----------------------------------------------------------------------------
pqPointSourceWidget::~pqPointSourceWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqPointSourceWidget::resetBounds(double input_bounds[6])
{
  this->Superclass::resetBounds(input_bounds);

  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double min_diameter = input_bounds[1]-input_bounds[0];
  min_diameter = qMin(min_diameter, input_bounds[3]-input_bounds[2]);
  min_diameter = qMin(min_diameter, input_bounds[5]-input_bounds[4]);
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("Radius"));
  if (dvp)
    {
    dvp->SetElement(0, min_diameter * 0.1);
    }
  widget->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqPointSourceWidget::setControlledProperty(const char* function,
  vtkSMProperty* _property)
{
  if (strcmp(function, "NumberOfPoints") == 0)
    {
    this->Implementation->Links.addPropertyLink(
      this->Implementation->Controls.NumberOfPoints, "value", 
      SIGNAL(valueChanged(int)),
      this->getWidgetProxy(), this->getWidgetProxy()->GetProperty("NumberOfPoints"));
    }
  else if (strcmp(function, "Radius") == 0)
    {
    this->Implementation->Links.addPropertyLink(
      this->Implementation->Controls.Radius, "text", 
      SIGNAL(textChanged(QString)),
      this->getWidgetProxy(), this->getWidgetProxy()->GetProperty("Radius"));
    }
  this->Superclass::setControlledProperty(function, _property);
}

