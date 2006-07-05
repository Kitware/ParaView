/*=========================================================================

   Program:   ParaQ
   Module:    pqStreamTracerPanel.cxx

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

#include "pqApplicationCore.h"
#include "pqLineWidget.h"
#include "pqNamedWidgets.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPointSourceWidget.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqStreamTracerPanel.h"

#include "ui_pqStreamTracerControls.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxyProperty.h>

#include <QFrame>
#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqStreamTracerPanel::pqImplementation

class pqStreamTracerPanel::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    PointSourceWidget(new pqPointSourceWidget(parent)),
    LineWidget(0 /*new pqLineWidget(parent)*/)
  {
  }

  ~pqImplementation()
  {
    delete this->LineWidget;
    delete this->PointSourceWidget;
  }
  
  /// Provides a UI for managing a vtkPointSource
  pqPointSourceWidget* const PointSourceWidget;
  /// Manages a 3D line widget, plus Qt controls  
  pqLineWidget* const LineWidget;
  /// Provides a container for Qt controls
  QWidget ControlsContainer;
  /// Provides the remaining Qt controls for the panel
  Ui::pqStreamTracerControls Controls;
};

pqStreamTracerPanel::pqStreamTracerPanel(QWidget* p) :
  base(p),
  Implementation(new pqImplementation(this))
{
  QVBoxLayout* const panel_layout = new QVBoxLayout();

  this->Implementation->Controls.setupUi(
    &this->Implementation->ControlsContainer);
  panel_layout->addWidget(&this->Implementation->ControlsContainer);

  QFrame* const separator = new QFrame();
  separator->setFrameShape(QFrame::HLine);
  panel_layout->addWidget(separator);
  
  if(this->Implementation->PointSourceWidget)
    {
    panel_layout->addWidget(this->Implementation->PointSourceWidget);

    connect(
      this->Implementation->PointSourceWidget,
      SIGNAL(widgetChanged()),
      this->getPropertyManager(),
      SLOT(propertyChanged()));
    }

  if(this->Implementation->LineWidget)
    {
    panel_layout->addWidget(this->Implementation->LineWidget);

    connect(
      this->Implementation->LineWidget,
      SIGNAL(widgetChanged()),
      this->getPropertyManager(),
      SLOT(propertyChanged()));
    }
  
  panel_layout->addStretch();
  
  this->setLayout(panel_layout);
  
  connect(
    this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
    
  connect(
    this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

pqStreamTracerPanel::~pqStreamTracerPanel()
{
  this->setProxy(0);
  delete this->Implementation;
}

void pqStreamTracerPanel::onAccepted()
{
  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->accept();
    }
}

void pqStreamTracerPanel::onRejected()
{
  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->reset();
    }
}

void pqStreamTracerPanel::setProxyInternal(pqSMProxy p)
{
  base::setProxyInternal(p);

  pqSMProxy reference_proxy = this->Proxy;
  pqSMProxy controlled_proxy;

  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("Source")))
      {
      controlled_proxy = source_property->GetProxy(0);
      }
    }
   
  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->setDataSources(
      reference_proxy, controlled_proxy);
    }
    
  if(this->Implementation->LineWidget)
    {
    this->Implementation->LineWidget->setReferenceProxy(reference_proxy);
    }
}

void pqStreamTracerPanel::select()
{
  pqNamedWidgets::link(
    &this->Implementation->ControlsContainer, this->Proxy, this->PropertyManager);

  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->showWidget(this->PropertyManager);
    }
    
  if(this->Implementation->LineWidget)
    {
    this->Implementation->LineWidget->showWidget();
    }
}

void pqStreamTracerPanel::deselect()
{
  pqNamedWidgets::unlink(
    &this->Implementation->ControlsContainer, this->Proxy, this->PropertyManager);

  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->hideWidget(this->PropertyManager);
    }

  if(this->Implementation->LineWidget)
    {    
    this->Implementation->LineWidget->hideWidget();
    }
}
