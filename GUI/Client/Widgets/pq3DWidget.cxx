/*=========================================================================

   Program: ParaView
   Module:    pq3DWidget.cxx

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
#include "pq3DWidget.h"

// ParaView Server Manager includes.
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMNew3DWidgetProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMIntVectorProperty.h"

// Qt includes.
#include <QtDebug>

// ParaView GUI includes.
#include "pqApplicationCore.h"
#include "pqRenderModule.h"

class pq3DWidgetInternal
{
public:
  vtkSmartPointer<vtkSMProxy> ReferenceProxy;
  vtkSmartPointer<vtkSMProxy> ControlledProxy;
  vtkSmartPointer<vtkSMNew3DWidgetProxy> WidgetProxy;

  QMap<vtkSmartPointer<vtkSMProperty>, vtkSmartPointer<vtkSMProperty> > PropertyMap;

  static QMap<vtkSMProxy*, bool> Visibility;
};
QMap<vtkSMProxy*, bool> pq3DWidgetInternal::Visibility;

//-----------------------------------------------------------------------------
pq3DWidget::pq3DWidget(QWidget* _p): QWidget(_p)
{
  this->Internal = new pq3DWidgetInternal;
  this->Ignore3DWidget = false;
  this->IgnorePropertyChange = false;
}

//-----------------------------------------------------------------------------
pq3DWidget::~pq3DWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setWidgetProxy(vtkSMNew3DWidgetProxy* proxy)
{
  this->Internal->WidgetProxy = proxy;
}

//-----------------------------------------------------------------------------
vtkSMNew3DWidgetProxy* pq3DWidget::getWidgetProxy() const
{
  return this->Internal->WidgetProxy;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setReferenceProxy(vtkSMProxy* proxy)
{
  this->Internal->ReferenceProxy = proxy;
  if (proxy && !this->Internal->Visibility.contains(proxy))
    {
    this->Internal->Visibility.insert(proxy, true);
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pq3DWidget::getReferenceProxy() const
{
  return this->Internal->ReferenceProxy;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProxy(vtkSMProxy* proxy)
{
  this->Internal->PropertyMap.clear();
  this->Internal->ControlledProxy = proxy;
  if (!proxy)
    {
    return;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkPVXMLElement* hints = pxm->GetHints(proxy->GetXMLGroup(), 
    proxy->GetXMLName());
  if (!hints)
    {
    qDebug() << "pq3DWidget cannot control a proxy that does "
      << "not provide Hints.";
    return ;
    }
  unsigned int max = hints->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* elem = hints->GetNestedElement(cc);
    if (elem && elem->GetName() && strcmp(elem->GetName(), "Widget") == 0)
      {
      unsigned int max_props = elem->GetNumberOfNestedElements();
      for (unsigned int i=0; i < max_props; i++)
        {
        vtkPVXMLElement* propElem = elem->GetNestedElement(i);
        this->setControlledProperty(propElem->GetAttribute("name"),
          proxy->GetProperty(propElem->GetAttribute("controls")));
        }
      break;
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pq3DWidget::getControlledProxy() const
{
  return this->Internal->ControlledProxy;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProperty(const char* function,
  vtkSMProperty* controlled_property)
{
  this->Internal->PropertyMap.insert(
    this->Internal->WidgetProxy->GetProperty(function),
    controlled_property);
}

//-----------------------------------------------------------------------------
void pq3DWidget::accept()
{
  this->IgnorePropertyChange = true;
  this->Internal->WidgetProxy->UpdatePropertyInformation();
  QMap<vtkSmartPointer<vtkSMProperty>, vtkSmartPointer<vtkSMProperty> >::const_iterator
    iter;
  for (iter = this->Internal->PropertyMap.constBegin() ;
    iter != this->Internal->PropertyMap.constEnd(); 
    ++iter)
    {
    vtkSMProperty* info_prop = iter.key()->GetInformationProperty();
    if (!info_prop)
      {
      qDebug() << " Some property has no information property. "
        "3Dwidgets cannot work.";
      continue;
      }
    iter.value()->Copy(info_prop);
    }
  if (this->Internal->ControlledProxy)
    {
    this->Internal->ControlledProxy->UpdateVTKObjects();
    }
  this->IgnorePropertyChange = false;
}

//-----------------------------------------------------------------------------
void pq3DWidget::reset()
{
  QMap<vtkSmartPointer<vtkSMProperty>, vtkSmartPointer<vtkSMProperty> >::const_iterator
    iter;
  for (iter = this->Internal->PropertyMap.constBegin() ;
    iter != this->Internal->PropertyMap.constEnd(); 
    ++iter)
    {
    iter.key()->Copy(iter.value());
    }

  if (this->Internal->WidgetProxy)
    {
    this->Internal->WidgetProxy->UpdateVTKObjects();
    pqApplicationCore::instance()->render();
    }
}

//-----------------------------------------------------------------------------
bool pq3DWidget::widgetVisibile() const
{
  if (!this->Internal->ReferenceProxy || 
    !this->Internal->Visibility.contains(this->Internal->ReferenceProxy))
    {
    return false;
    }
  return this->Internal->Visibility[this->Internal->ReferenceProxy];
}

//-----------------------------------------------------------------------------
void pq3DWidget::select()
{
  if (this->widgetVisibile())
    {
    this->set3DWidgetVisibility(true);
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::deselect()
{
  if (this->widgetVisibile())
    {
    this->set3DWidgetVisibility(false);
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::showWidget()
{
  this->set3DWidgetVisibility(true);
  this->Internal->Visibility.insert(this->Internal->ReferenceProxy, true);
}

//-----------------------------------------------------------------------------
void pq3DWidget::hideWidget()
{
  this->set3DWidgetVisibility(false);
    this->Internal->Visibility.insert(this->Internal->ReferenceProxy, false);
}

//-----------------------------------------------------------------------------
void pq3DWidget::set3DWidgetVisibility(bool visible)
{
  if(this->Internal->WidgetProxy)
    {
    this->Ignore3DWidget = true;
    
    if(vtkSMIntVectorProperty* const visibility =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Internal->WidgetProxy->GetProperty("Visibility")))
      {
      visibility->SetElement(0, visible);
      }

    if(vtkSMIntVectorProperty* const enabled =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Internal->WidgetProxy->GetProperty("Enabled")))
      {
      enabled->SetElement(0, visible);
      }

    this->Internal->WidgetProxy->UpdateVTKObjects();
    pqApplicationCore::instance()->render();
    this->Ignore3DWidget = false;
    }
}
