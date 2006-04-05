/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

#include "pqPipelineServer.h"

#include "pqMultiView.h"
#include "pqPipelineObject.h"
#include "pqPipelineWindow.h"
#include "pqServer.h"
#include "pqXMLUtil.h"

#include <QHash>
#include <QList>
#include <QString>

#include "vtkPVXMLElement.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"


class pqPipelineServerInternal
{
public:
  pqPipelineServerInternal();
  ~pqPipelineServerInternal() {}

  QList<pqPipelineObject *> Sources;
  QList<pqPipelineWindow *> Windows;
  QHash<vtkSMProxy *, pqPipelineObject *> Objects;
};


pqPipelineServerInternal::pqPipelineServerInternal()
  : Sources(), Windows(), Objects()
{
}


pqPipelineServer::pqPipelineServer()
{
  this->Internal = new pqPipelineServerInternal();
  this->Server = 0;
}

pqPipelineServer::~pqPipelineServer()
{
  this->ClearPipelines();
  if(this->Internal)
    delete this->Internal;
}

void pqPipelineServer::SaveState(vtkPVXMLElement *root, pqMultiView *multiView)
{
  if(!root || !this->Server)
    {
    return;
    }

  vtkPVXMLElement *element = 0;
  if(multiView && this->Internal)
    {
    // Save the window information for the server.
    QWidget *widget = 0;
    QList<pqPipelineWindow *>::Iterator iter = this->Internal->Windows.begin();
    for( ; iter != this->Internal->Windows.end(); ++iter)
      {
      widget = (*iter)->GetWidget();
      element = vtkPVXMLElement::New();
      element->SetName("Window");
      element->AddAttribute("className", widget->metaObject()->className());
      element->AddAttribute("windowID", ParaQ::GetStringFromIntList(
          multiView->indexOf(widget->parentWidget())).toAscii().data());
      element->AddAttribute("name", widget->windowTitle().toAscii().data());
      root->AddNestedElement(element);
      element->Delete();
      }

    // Save display proxy to window relationships. Each object in the
    // map should have a display proxy.
    vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator jter =
        this->Internal->Objects.begin();
    for( ; jter != this->Internal->Objects.end(); ++jter)
      {
      widget = (*jter)->GetParent()->GetWidget();
      element = vtkPVXMLElement::New();
      element->SetName("Display");
      element->AddAttribute("name", proxyManager->GetProxyName(
        "displays", (*jter)->GetDisplayProxy()));
      element->AddAttribute("windowID", ParaQ::GetStringFromIntList(
          multiView->indexOf(widget->parentWidget())).toAscii().data());
      element->AddAttribute("proxyName",
          (*jter)->GetProxyName().toAscii().data());
      root->AddNestedElement(element);
      element->Delete();
      }
    }

  // Save the pipeline information.
  element = vtkPVXMLElement::New();
  element->SetName("ServerManagerState");
  // TODO: Save state and multiple connections....how to they interact?
  vtkSMObject::GetProxyManager()->SaveState(element);
  root->AddNestedElement(element);
  element->Delete();
}

pqPipelineObject *pqPipelineServer::AddSource(vtkSMProxy *source)
{
  pqPipelineObject *object = 0;
  if(this->Internal && source)
    {
    object = new pqPipelineObject(source, pqPipelineObject::Source);
    if(object)
      {
      this->Internal->Objects.insert(source, object);
      this->Internal->Sources.append(object);
      }
    }

  return object;
}

pqPipelineObject *pqPipelineServer::AddFilter(vtkSMProxy *filter)
{
  pqPipelineObject *object = 0;
  if(this->Internal && filter)
    {
    object = new pqPipelineObject(filter, pqPipelineObject::Filter);
    if(object)
      this->Internal->Objects.insert(filter, object);
    }

  return object;
}

pqPipelineObject *pqPipelineServer::AddCompoundProxy(vtkSMProxy *proxy)
{
  pqPipelineObject *object = 0;
  if(this->Internal && proxy)
    {
    object = new pqPipelineObject(proxy, pqPipelineObject::Bundle);
    if(object)
      this->Internal->Objects.insert(proxy, object);
    }

  return object;
}

pqPipelineWindow *pqPipelineServer::AddWindow(QWidget *window)
{
  pqPipelineWindow *object = 0;
  if(this->Internal && window)
    {
    object = new pqPipelineWindow(window);
    if(object)
      this->Internal->Windows.append(object);
    }

  return object;
}

pqPipelineObject *pqPipelineServer::GetObject(vtkSMProxy *proxy) const
{
  if(this->Internal)
    {
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator iter =
        this->Internal->Objects.find(proxy);
    if(iter != this->Internal->Objects.end())
      return *iter;
    }

  return 0;
}

pqPipelineWindow *pqPipelineServer::GetWindow(QWidget *window) const
{
  if(this->Internal)
    {
    QList<pqPipelineWindow *>::Iterator iter = this->Internal->Windows.begin();
    for( ; iter != this->Internal->Windows.end(); ++iter)
      {
      if(*iter && (*iter)->GetWidget() == window)
        return *iter;
      }
    }

  return 0;
}

bool pqPipelineServer::RemoveObject(vtkSMProxy *proxy)
{
  if(this->Internal && proxy)
    {
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator iter =
        this->Internal->Objects.find(proxy);
    if(iter != this->Internal->Objects.end())
      {
      pqPipelineObject *object = *iter;
      this->Internal->Objects.erase(iter);
      if(object && object->GetType() == pqPipelineObject::Source)
        {
        this->Internal->Sources.removeAll(object);
        }

      this->UnregisterObject(object);
      delete object;
      return true;
      }
    }

  return false;
}

bool pqPipelineServer::RemoveWindow(QWidget *window)
{
  if(this->Internal && window)
    {
    QList<pqPipelineWindow *>::Iterator iter = this->Internal->Windows.begin();
    for( ; iter != this->Internal->Windows.end(); ++iter)
      {
      if(*iter && (*iter)->GetWidget() == window)
        {
        // Clean up all objects connected with the window.
        QHash<vtkSMProxy *, pqPipelineObject *>::Iterator jter =
            this->Internal->Objects.begin();
        while(jter != this->Internal->Objects.end())
          {
          if(*jter && (*jter)->GetParent() &&
              (*jter)->GetParent()->GetWidget() == window)
            {
            pqPipelineObject *object = *jter;
            jter = this->Internal->Objects.erase(jter);
            if(object->GetType() == pqPipelineObject::Source)
              {
              this->Internal->Sources.removeAll(object);
              }

            this->UnregisterObject(object);
            delete object;
            }
          else
            {
            ++jter;
            }
          }
        
        delete *iter;
        this->Internal->Windows.erase(iter);

        return true;
        }
      }
    }

  return false;
}

int pqPipelineServer::GetSourceCount() const
{
  if(this->Internal)
    return this->Internal->Sources.size();
  return 0;
}

pqPipelineObject *pqPipelineServer::GetSource(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Sources.size())
    return this->Internal->Sources[index];
  return 0;
}

int pqPipelineServer::GetWindowCount() const
{
  if(this->Internal)
    return this->Internal->Windows.size();
  return 0;
}

pqPipelineWindow *pqPipelineServer::GetWindow(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Windows.size())
    return this->Internal->Windows[index];
  return 0;
}

void pqPipelineServer::ClearPipelines()
{
  if(this->Internal)
    {
    // Clean up the objects in the map.
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator iter =
        this->Internal->Objects.begin();
    for( ; iter != this->Internal->Objects.end(); ++iter)
      {
      if(*iter)
        {
        this->UnregisterObject(*iter);
        delete *iter;
        *iter = 0;
        }
      }

    // Clean up the window objects.
    QList<pqPipelineWindow *>::Iterator jter = this->Internal->Windows.begin();
    for( ; jter != this->Internal->Windows.end(); ++jter)
      {
      if(*jter)
        {
        delete *jter;
        *jter = 0;
        }
      }

    this->Internal->Sources.clear();
    this->Internal->Windows.clear();
    this->Internal->Objects.clear();
    }
}

void pqPipelineServer::UnregisterObject(pqPipelineObject *object)
{
  if(this->Server)
    {
    vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
    vtkSMDisplayProxy *display = object->GetDisplayProxy();
    vtkSMProxyProperty *property = vtkSMProxyProperty::SafeDownCast(
        display->GetProperty("LookupTable"));
    if(property && property->GetNumberOfProxies())
      {
      vtkSMLookupTableProxy *lookup = vtkSMLookupTableProxy::SafeDownCast(
          property->GetProxy(0));
      proxyManager->UnRegisterProxy(proxyManager->GetProxyName("lookup_tables",
          lookup));
      }

    object->SetDisplayProxy(0);
    proxyManager->UnRegisterProxy(proxyManager->GetProxyName("displays",
        display));
    object->SetProxy(0);
    proxyManager->UnRegisterProxy(object->GetProxyName().toAscii().data());
    object->SetProxyName("");
    }
}


