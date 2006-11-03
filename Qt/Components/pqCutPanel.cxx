/*=========================================================================

   Program: ParaView
   Module:    pqCutPanel.cxx

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

#include "pqApplicationCore.h"
#include "pqCutPanel.h"
#include "pqImplicitPlaneWidget.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqSampleScalarWidget.h"
#include "pqServerManagerModel.h"

#include <pqCollapsedGroup.h>

#include <vtkPVXMLElement.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyListDomain.h>

#include <QVBoxLayout>

// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

QString pqCutPanelInterface::name() const
{
  return "Cut";
}

pqObjectPanel* pqCutPanelInterface::createPanel(pqProxy* proxy, QWidget* p)
{
  return new pqCutPanel(proxy, p);
}

bool pqCutPanelInterface::canCreatePanel(pqProxy* proxy) const
{
  return (proxy->getProxy()->GetXMLName() == QString("Cut") 
    && proxy->getProxy()->GetXMLGroup() == QString("filters"));
}

Q_EXPORT_PLUGIN(pqCutPanelInterface)

//////////////////////////////////////////////////////////////////////////////
// pqCutPanel::pqImplementation

class pqCutPanel::pqImplementation
{
public:
  pqImplementation() :
    ImplicitPlaneWidget(),
    SampleScalarWidget()
  {
  }
  
  /// Manages a 3D implicit plane widget, plus Qt controls  
  pqImplicitPlaneWidget ImplicitPlaneWidget;
  /// Controls the number and position of "slices"
  pqSampleScalarWidget SampleScalarWidget;
};

pqCutPanel::pqCutPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p),
  Implementation(new pqImplementation())
{
  pqCollapsedGroup* const group1 = new pqCollapsedGroup(this);
  group1->setTitle(tr("Implicit Plane"));
  QVBoxLayout* l = new QVBoxLayout(group1);
  l->addWidget(&this->Implementation->ImplicitPlaneWidget);

  pqCollapsedGroup* const group2 = new pqCollapsedGroup(this);
  group2->setTitle(tr("Slices"));
  l = new QVBoxLayout(group2);
  l->addWidget(&this->Implementation->SampleScalarWidget);
  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->addWidget(group1);
  panel_layout->addWidget(group2);
  panel_layout->addStretch();
  
  QObject::connect(this, SIGNAL(renderModuleChanged(pqRenderModule*)),
                   &this->Implementation->ImplicitPlaneWidget,
                   SLOT(setRenderModule(pqRenderModule*)));

  connect(
    &this->Implementation->ImplicitPlaneWidget,
    SIGNAL(widgetStartInteraction()),
    &this->Implementation->ImplicitPlaneWidget,
    SLOT(showPlane()));
  
  connect(
    &this->Implementation->ImplicitPlaneWidget,
    SIGNAL(widgetEndInteraction()),
    &this->Implementation->ImplicitPlaneWidget,
    SLOT(hidePlane()));
  
  connect(
    &this->Implementation->ImplicitPlaneWidget,
    SIGNAL(widgetChanged()),
    this->propertyManager(),
    SLOT(propertyChanged()));
    
  connect(
    &this->Implementation->SampleScalarWidget,
    SIGNAL(samplesChanged()),
    this->propertyManager(),
    SLOT(propertyChanged()));
  
  connect(
    this->propertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
    
  connect(
    this->propertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));

  // Setup the implicit plane widget ...
  pqProxy* reference_proxy = this->proxy();
  pqSMProxy controlled_proxy = NULL;
   
  if(vtkSMProxyProperty* const cut_function_property = vtkSMProxyProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("CutFunction")))
    {
    if (cut_function_property->GetNumberOfProxies() == 0)
      {
      vtkSMProxyListDomain* pld = vtkSMProxyListDomain::SafeDownCast(
        cut_function_property->GetDomain("proxy_list"));
      if (pld)
        {
        cut_function_property->AddProxy(pld->GetProxy(0));
        this->proxy()->getProxy()->UpdateVTKObjects();
        }
      }
    controlled_proxy = cut_function_property->GetProxy(0);
    controlled_proxy->UpdateVTKObjects();
    }

  this->Implementation->ImplicitPlaneWidget.setReferenceProxy(reference_proxy);
  this->Implementation->ImplicitPlaneWidget.setControlledProxy(controlled_proxy);

  if (controlled_proxy)
    {
    vtkPVXMLElement* hints = controlled_proxy->GetHints();
    for (unsigned int cc=0; cc <hints->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* elem = hints->GetNestedElement(cc);
      if (QString("PropertyGroup") == elem->GetName() && 
        QString("Plane") == elem->GetAttribute("type"))
        {
        this->Implementation->ImplicitPlaneWidget.setHints(elem);
        break;
        }
      }
    }

  // Setup the sample scalar widget ...
  this->Implementation->SampleScalarWidget.setDataSources(
    this->proxy()->getProxy(),
    vtkSMDoubleVectorProperty::SafeDownCast(this->proxy()->getProxy()->GetProperty("ContourValues")));
}

pqCutPanel::~pqCutPanel()
{
  delete this->Implementation;
}

void pqCutPanel::onAccepted()
{
  this->Implementation->ImplicitPlaneWidget.accept();
  this->Implementation->SampleScalarWidget.accept();
      
  // If this is the first time we've been accepted since our creation, hide the source
  if(pqPipelineFilter* const pipeline_filter =
    qobject_cast<pqPipelineFilter*>(this->proxy()))
    {
    if(0 == pipeline_filter->getDisplayCount())
      {
      for(int i = 0; i != pipeline_filter->getInputCount(); ++i)
        {
        if(pqPipelineSource* const pipeline_source = pipeline_filter->getInput(i))
          {
          for(int j = 0; j != pipeline_source->getDisplayCount(); ++j)
            {
            pqPipelineDisplay* const pipeline_display =
              qobject_cast<pqPipelineDisplay*>(pipeline_source->getDisplay(j));
            if (pipeline_display)
              {
              vtkSMDataObjectDisplayProxy* const display_proxy =
                pipeline_display->getDisplayProxy();

              display_proxy->SetVisibilityCM(false);
              }
            }
          }
        }
      }
    }
}

void pqCutPanel::onRejected()
{
  this->Implementation->ImplicitPlaneWidget.reset();
  this->Implementation->SampleScalarWidget.reset();
}

void pqCutPanel::select()
{
  this->Implementation->ImplicitPlaneWidget.select();
}

void pqCutPanel::deselect()
{
  this->Implementation->ImplicitPlaneWidget.deselect();
}

pqImplicitPlaneWidget* pqCutPanel::getImplicitPlaneWidget()
{
  return &this->Implementation->ImplicitPlaneWidget;
}
