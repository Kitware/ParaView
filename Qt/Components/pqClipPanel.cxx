/*=========================================================================

   Program: ParaView
   Module:    pqClipPanel.cxx

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
#include "pqClipPanel.h"
#include "pqImplicitPlaneWidget.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"

#include <pqCollapsedGroup.h>

#include <vtkPVXMLElement.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxyProperty.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqClipPanel::pqImplementation

class pqClipPanel::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    InsideOutWidget(tr("Inside Out")),
    ImplicitPlaneWidget(parent)
  {
  }
  
  /// Provides a Qt control for the "Inside Out" property of the Clip filter
  QCheckBox InsideOutWidget;
  /// Manages a 3D implicit plane widget, plus Qt controls  
  pqImplicitPlaneWidget ImplicitPlaneWidget;
};

pqClipPanel::pqClipPanel(QWidget* p) :
  Superclass(p),
  Implementation(new pqImplementation(this))
{
  pqCollapsedGroup* const group1 = new pqCollapsedGroup(tr("Clip"));
  group1->setWidget(&this->Implementation->InsideOutWidget);

  pqCollapsedGroup* const group2 = new pqCollapsedGroup(tr("Implicit Plane"));
  group2->setWidget(&this->Implementation->ImplicitPlaneWidget);
  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->setMargin(0);
  panel_layout->setSpacing(0);
  panel_layout->addWidget(group1);
  panel_layout->addWidget(group2);
  panel_layout->addStretch();
  
  this->setLayout(panel_layout);

  QObject::connect(this, SIGNAL(renderModuleChanged(pqRenderModule*)),
                   &this->Implementation->ImplicitPlaneWidget,
                   SLOT(setRenderModule(pqRenderModule*)));

  connect(&this->Implementation->ImplicitPlaneWidget, SIGNAL(widgetChanged()), this, SLOT(onWidgetChanged()));
  connect(this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

pqClipPanel::~pqClipPanel()
{
  this->setProxy(0);
  delete this->Implementation;
}

void pqClipPanel::onWidgetChanged()
{
  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqClipPanel::onAccepted()
{
  this->Implementation->ImplicitPlaneWidget.accept();

  // If this is the first time we've been accepted since our creation, hide the source
  if(pqPipelineFilter* const pipeline_filter = 
           qobject_cast<pqPipelineFilter*>(this->Proxy))
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
              pipeline_source->getDisplay(j);
              
            vtkSMDataObjectDisplayProxy* const display_proxy =
              pipeline_display->getDisplayProxy();

            display_proxy->SetVisibilityCM(false);
            }
          }
        }
      }
    }
}

void pqClipPanel::onRejected()
{
  this->Implementation->ImplicitPlaneWidget.reset();
}

void pqClipPanel::setProxyInternal(pqProxy* p)
{
  if(this->Proxy)
    {
    this->PropertyManager->unregisterLink(
      &this->Implementation->InsideOutWidget, "checked", SIGNAL(toggled(bool)),
      this->Proxy->getProxy(), 
      this->Proxy->getProxy()->GetProperty("InsideOut"));
    }

  Superclass::setProxyInternal(p);
 
  pqProxy* reference_proxy = p;
  vtkSMProxy* controlled_proxy = NULL;
   
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->getProxy()->GetProperty("ClipFunction")))
      {
      controlled_proxy = clip_function_property->GetProxy(0);
      }
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

  if(this->Proxy)
    {
    this->PropertyManager->registerLink(
      &this->Implementation->InsideOutWidget, "checked", SIGNAL(toggled(bool)),
      this->Proxy->getProxy(), 
      this->Proxy->getProxy()->GetProperty("InsideOut"));
    }
}

void pqClipPanel::select()
{
  this->Implementation->ImplicitPlaneWidget.select();
}

void pqClipPanel::deselect()
{
  this->Implementation->ImplicitPlaneWidget.deselect();
}
