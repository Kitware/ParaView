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
#include "pqStreamTracerPanel.h"

#include "pqApplicationCore.h"
#include "pqLineWidget.h"
#include "pqNamedWidgets.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPointSourceWidget.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

#include "ui_pqStreamTracerControls.h"

#include <pqCollapsedGroup.h>

#include <vtkPVXMLElement.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyProperty.h>

#include <QVBoxLayout>

// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

QString pqStreamTracerPanelInterface::name() const
{
  return "StreamTracer";
}

pqObjectPanel* pqStreamTracerPanelInterface::createPanel(QWidget* p)
{
  return new pqStreamTracerPanel(p);
}

Q_EXPORT_PLUGIN(pqStreamTracerPanelInterface)
Q_IMPORT_PLUGIN(pqStreamTracerPanelInterface)

//////////////////////////////////////////////////////////////////////////////
// pqStreamTracerPanel::pqImplementation

class pqStreamTracerPanel::pqImplementation
{
public:
  pqImplementation() :
    PointSourceWidget(new pqPointSourceWidget()),
    LineWidget(0 /*new pqLineWidget()*/)
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
  Implementation(new pqImplementation())
{
  this->Implementation->PointSourceWidget->setRenderModule(
    this->getRenderModule());

  QObject::connect(this, SIGNAL(renderModuleChanged(pqRenderModule*)),
                   this->Implementation->PointSourceWidget,
                   SLOT(setRenderModule(pqRenderModule*)));

  this->Implementation->Controls.setupUi(
    &this->Implementation->ControlsContainer);
    
  pqCollapsedGroup* const group1 = new pqCollapsedGroup(tr("Stream Tracer"));
  group1->setWidget(&this->Implementation->ControlsContainer);

  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->setMargin(0);
  panel_layout->setSpacing(0);
  panel_layout->addWidget(group1);
  
  if(this->Implementation->PointSourceWidget)
    {
    pqCollapsedGroup* const group2 = new pqCollapsedGroup(tr("Point Source"));
    group2->setWidget(this->Implementation->PointSourceWidget);
  
    panel_layout->addWidget(group2);

    connect(
      this->Implementation->PointSourceWidget,
      SIGNAL(widgetChanged()),
      this->getPropertyManager(),
      SLOT(propertyChanged()));
    }

  if(this->Implementation->LineWidget)
    {
    pqCollapsedGroup* const group2 = new pqCollapsedGroup(tr("Line Source"));
    group2->setWidget(this->Implementation->LineWidget);
  
    panel_layout->addWidget(group2);

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

void pqStreamTracerPanel::setProxyInternal(pqProxy* p)
{
  if(this->Proxy)
    {
    pqNamedWidgets::unlink(
      &this->Implementation->ControlsContainer, this->Proxy->getProxy(), this->PropertyManager);
    }

  base::setProxyInternal(p);

  pqProxy* reference_proxy = this->Proxy;
  pqSMProxy controlled_proxy = NULL;

  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->getProxy()->GetProperty("Source")))
      {
      controlled_proxy = pqSMAdaptor::getProxyProperty(source_property);
      if (source_property->GetNumberOfProxies() == 0)
        {
        source_property->AddProxy(controlled_proxy);
        if(vtkSMIntVectorProperty* const number_of_points =
          vtkSMIntVectorProperty::SafeDownCast(
            controlled_proxy->GetProperty("NumberOfPoints")))
          {
          number_of_points->SetNumberOfElements(1);
          number_of_points->SetElement(0, 100);
          }
        controlled_proxy->UpdateVTKObjects();
        }
      }
    }
   
  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->setReferenceProxy(
      reference_proxy);
    this->Implementation->PointSourceWidget->setControlledProxy(
      controlled_proxy);
    if (controlled_proxy)
      {
      vtkPVXMLElement* hints = controlled_proxy->GetHints();
      for (unsigned int cc=0; cc <hints->GetNumberOfNestedElements(); cc++)
        {
        vtkPVXMLElement* elem = hints->GetNestedElement(cc);
        if (QString("PropertyGroup") == elem->GetName() && 
          QString("PointSource") == elem->GetAttribute("type"))
          {
          this->Implementation->PointSourceWidget->setHints(elem);
          break;
          }
        }
      }
    }
    
  if(this->Implementation->LineWidget)
    {
    this->Implementation->LineWidget->setReferenceProxy(reference_proxy);
    }
    
  if(this->Proxy)
    {
    pqNamedWidgets::link(
      &this->Implementation->ControlsContainer, this->Proxy->getProxy(), this->PropertyManager);
    }
}

void pqStreamTracerPanel::select()
{
  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->select();
    }
    
  if(this->Implementation->LineWidget)
    {
    this->Implementation->LineWidget->showWidget();
    }
}

void pqStreamTracerPanel::deselect()
{
  if(this->Implementation->PointSourceWidget)
    {
    this->Implementation->PointSourceWidget->deselect();
    }

  if(this->Implementation->LineWidget)
    {    
    this->Implementation->LineWidget->hideWidget();
    }
}
