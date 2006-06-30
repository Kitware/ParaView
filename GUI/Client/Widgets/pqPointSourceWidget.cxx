/*=========================================================================

   Program:   ParaQ
   Module:    pqPointSourceWidget.cxx

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

#include "pqHandleWidget.h"
#include "pqNamedWidgets.h"
#include "pqPointSourceWidget.h"

#include "ui_pqPointSourceControls.h"

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyProperty.h>

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
  pqImplementation(QWidget* parent) :
    HandleWidget(parent)
  {
  }
  
  pqHandleWidget HandleWidget;
  QWidget ControlsContainer;
  Ui::pqPointSourceControls Controls;
  pqSMProxy ControlledProxy;
};

/////////////////////////////////////////////////////////////////////////
// pqPointSourceWidget

pqPointSourceWidget::pqPointSourceWidget(QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation(this))
{
  this->Implementation->Controls.setupUi(
    &this->Implementation->ControlsContainer);

  QVBoxLayout* const widget_layout = new QVBoxLayout();
  widget_layout->addWidget(&this->Implementation->HandleWidget);
  widget_layout->addWidget(&this->Implementation->ControlsContainer);
  
  this->setLayout(widget_layout);

  this->Implementation->Controls.NumberOfPoints->setValidator(new QIntValidator(0, 999999, this));
  this->Implementation->Controls.Radius->setValidator(new QDoubleValidator(0, 99999999, 12, this));

  connect(
    &this->Implementation->HandleWidget,
    SIGNAL(widgetChanged()),
    this,
    SIGNAL(widgetChanged()));
}

pqPointSourceWidget::~pqPointSourceWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqPointSourceWidget::setDataSources(pqSMProxy reference_proxy,
  pqSMProxy controlled_proxy)
{
  this->Implementation->HandleWidget.setDataSources(reference_proxy);
  this->Implementation->ControlledProxy = controlled_proxy;
}

void pqPointSourceWidget::showWidget(pqPropertyManager* property_manager)
{
  this->Implementation->HandleWidget.showWidget();
  
  pqNamedWidgets::link(
    &this->Implementation->ControlsContainer,
    this->Implementation->ControlledProxy,
    property_manager);
}

void pqPointSourceWidget::accept()
{
  // Get the current values from the 3D handle widget ...
  if(this->Implementation->ControlledProxy)
    {
    double center[3] = { 0, 0, 0 };
    
    this->Implementation->HandleWidget.getWidgetState(center);

    if(vtkSMDoubleVectorProperty* const center_property = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Implementation->ControlledProxy->GetProperty("Center")))
      {
      center_property->SetElements(center);
      }

    this->Implementation->ControlledProxy->UpdateVTKObjects();
    }
}

void pqPointSourceWidget::reset()
{
  // Restore the state of the 3D handle widget ...
  if(this->Implementation->ControlledProxy)
    {
    double center[3] = { 0, 0, 0 };

    if(vtkSMDoubleVectorProperty* const center_property = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Implementation->ControlledProxy->GetProperty("Center")))
      {
      center[0] = center_property->GetElement(0);
      center[1] = center_property->GetElement(1);
      center[2] = center_property->GetElement(2);
      }
      
    this->Implementation->HandleWidget.setWidgetState(center);
    }
}

void pqPointSourceWidget::hideWidget(pqPropertyManager* property_manager)
{
  this->Implementation->HandleWidget.hideWidget();

  pqNamedWidgets::unlink(
    &this->Implementation->ControlsContainer,
    this->Implementation->ControlledProxy,
    property_manager);
}

void pqPointSourceWidget::widgetChanged(const QString&)
{
  emit widgetChanged();
}
