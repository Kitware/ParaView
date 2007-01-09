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
#include <vtkSMProxyListDomain.h>
#include <vtkSMProxyProperty.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>

// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

QString pqClipPanelInterface::name() const
{
  return "Clip";
}

pqObjectPanel* pqClipPanelInterface::createPanel(pqProxy* proxy, QWidget* p)
{
  return new pqClipPanel(proxy, p);
}
 
bool pqClipPanelInterface::canCreatePanel(pqProxy* proxy) const
{
  return (QString("filters") == proxy->getProxy()->GetXMLGroup() &&
    QString("Clip") == proxy->getProxy()->GetXMLName());
}

Q_EXPORT_PLUGIN(pqClipPanelInterface)

//////////////////////////////////////////////////////////////////////////////
// pqClipPanel::pqImplementation

class pqClipPanel::pqImplementation
{
public:
  pqImplementation() :
    InsideOutWidget(tr("Inside Out")),
    ImplicitPlaneWidget()
  {
  }
  
  /// Provides a Qt control for the "Inside Out" property of the Clip filter
  QCheckBox InsideOutWidget;
  /// Manages a 3D implicit plane widget, plus Qt controls  
  pqImplicitPlaneWidget ImplicitPlaneWidget;
};

pqClipPanel::pqClipPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p),
  Implementation(new pqImplementation())
{
  pqCollapsedGroup* const group1 = new pqCollapsedGroup(this);
  group1->setTitle(tr("Clip"));
  QVBoxLayout* l = new QVBoxLayout(group1);
  l->addWidget(&this->Implementation->InsideOutWidget);

  pqCollapsedGroup* const group2 = new pqCollapsedGroup(this);
  group2->setTitle(tr("Implicit Plane"));
  l = new QVBoxLayout(group2);
  this->Implementation->ImplicitPlaneWidget.layout()->setMargin(0);
  l->addWidget(&this->Implementation->ImplicitPlaneWidget);
  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->addWidget(group1);
  panel_layout->addWidget(group2);
  panel_layout->addStretch();
  
  QObject::connect(this, SIGNAL(renderModuleChanged(pqRenderViewModule*)),
                   &this->Implementation->ImplicitPlaneWidget,
                   SLOT(setRenderModule(pqRenderViewModule*)));

  connect(&this->Implementation->ImplicitPlaneWidget, SIGNAL(widgetChanged()), this, SLOT(onWidgetChanged()));
  connect(this->propertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->propertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));

  pqProxy* reference_proxy = this->proxy();
  vtkSMProxy* controlled_proxy = NULL;
   
  if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("ClipFunction")))
    {
    if (clip_function_property->GetNumberOfProxies() == 0)
      {
      vtkSMProxyListDomain* pld = vtkSMProxyListDomain::SafeDownCast(
        clip_function_property->GetDomain("proxy_list"));
      if (pld)
        {
        clip_function_property->AddProxy(pld->GetProxy(0));
        this->proxy()->getProxy()->UpdateVTKObjects();
        }
      }
    controlled_proxy = clip_function_property->GetProxy(0);
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
  this->Implementation->ImplicitPlaneWidget.resetBounds();
  this->Implementation->ImplicitPlaneWidget.reset();

  this->propertyManager()->registerLink(
    &this->Implementation->InsideOutWidget, "checked", SIGNAL(toggled(bool)),
    this->proxy()->getProxy(), 
    this->proxy()->getProxy()->GetProperty("InsideOut"));
}

pqClipPanel::~pqClipPanel()
{
  delete this->Implementation;
}

void pqClipPanel::onWidgetChanged()
{
  // Signal the UI that there are changes to accept/reject ...
  this->propertyManager()->propertyChanged();
}

void pqClipPanel::onAccepted()
{
  this->Implementation->ImplicitPlaneWidget.accept();
}

void pqClipPanel::onRejected()
{
  this->Implementation->ImplicitPlaneWidget.reset();
}

void pqClipPanel::select()
{
  this->Implementation->ImplicitPlaneWidget.select();
}

void pqClipPanel::deselect()
{
  this->Implementation->ImplicitPlaneWidget.deselect();
}
