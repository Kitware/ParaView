/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineServer.cxx

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
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqXMLUtil.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>

#include "vtkPVXMLElement.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMProxyManager.h"


class pqPipelineServerInternal
{
public:
  pqPipelineServerInternal();
  ~pqPipelineServerInternal() {}

  QList<pqPipelineSource *> Sources;
  QList<QWidget *> Windows;
  QHash<vtkSMProxy *, pqPipelineSource *> Objects;
};


pqPipelineServerInternal::pqPipelineServerInternal()
  : Sources(), Windows(), Objects()
{
}


pqPipelineServer::pqPipelineServer()
{
  this->Internal = new pqPipelineServerInternal();
  this->Server = 0;

  // Set the pipeline model item type.
  this->setType(pqPipelineModel::Server);
}

pqPipelineServer::~pqPipelineServer()
{
  if(this->Internal)
    {
    // Clean up the objects in the map.
    QHash<vtkSMProxy *, pqPipelineSource *>::Iterator iter =
        this->Internal->Objects.begin();
    for( ; iter != this->Internal->Objects.end(); ++iter)
      {
      delete *iter;
      }

    delete this->Internal;
    }
}

void pqPipelineServer::SaveState(vtkPVXMLElement *root, pqMultiView *multiView)
{
  /* FIXME
  if(!root || !multiView || !this->Internal)
    {
    return;
    }

  // Save the window information for the server.
  QWidget *widget = 0;
  vtkPVXMLElement *element = 0;
  QList<QWidget *>::Iterator iter = this->Internal->Windows.begin();
  for( ; iter != this->Internal->Windows.end(); ++iter)
    {
    widget = *iter;
    element = vtkPVXMLElement::New();
    element->SetName("Window");
    element->AddAttribute("className", widget->metaObject()->className());
    element->AddAttribute("windowID", pqXMLUtil::GetStringFromIntList(
        multiView->indexOf(widget->parentWidget())).toAscii().data());
    element->AddAttribute("name", widget->windowTitle().toAscii().data());
    root->AddNestedElement(element);
    element->Delete();
    }

  // Save display proxy to window relationships. An object can have
  // more than one display. An object does not have to have a display.
  QHash<vtkSMProxy *, pqPipelineSource *>::Iterator jter =
      this->Internal->Objects.begin();
  for( ; jter != this->Internal->Objects.end(); ++jter)
    {
    (*jter)->getDisplay()->SaveState(root, multiView);
    }

  // Save the server manager state. This should save the proxy
  // information for this server connection.
  // TODO: Call the save state with the server connection ID.
  element = vtkPVXMLElement::New();
  element->SetName("ServerManagerState");
  vtkSMObject::GetProxyManager()->SaveState(element);
  root->AddNestedElement(element);
  element->Delete();
  */
}

void pqPipelineServer::AddSource(pqPipelineSource *source)
{
  if(this->Internal && source)
    {
    //source->setServer(this->Server); FIXME
    this->Internal->Objects.insert(source->getProxy(), source);
    this->Internal->Sources.append(source);
    }
}

void pqPipelineServer::AddObject(pqPipelineSource *source)
{
  if(this->Internal && source)
    {
    //source->setServer(this->Server); FIXME
    this->Internal->Objects.insert(source->getProxy(), source);
    }
}

void pqPipelineServer::RemoveObject(pqPipelineSource *source)
{
  if(this->Internal && source)
    {
    QHash<vtkSMProxy *, pqPipelineSource *>::Iterator iter =
        this->Internal->Objects.find(source->getProxy());
    if(iter != this->Internal->Objects.end())
      {
      this->Internal->Objects.erase(iter);
      }

    this->Internal->Sources.removeAll(source);
    // pqPipelineServer is not the class that should unregister any proxies.
    // It's not our responsibilty. We were not the one to register the proxy
    // in the first place, how can we be the one who unregisters it?
    // this->UnregisterObject(source);
    }
}

pqPipelineSource *pqPipelineServer::GetObject(vtkSMProxy *proxy) const
{
  if(this->Internal)
    {
    QHash<vtkSMProxy *, pqPipelineSource *>::Iterator iter =
        this->Internal->Objects.find(proxy);
    if(iter != this->Internal->Objects.end())
      {
      return *iter;
      }
    }

  return 0;
}

int pqPipelineServer::GetSourceCount() const
{
  if(this->Internal)
    {
    return this->Internal->Sources.size();
    }

  return 0;
}

pqPipelineSource *pqPipelineServer::GetSource(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Sources.size())
    {
    return this->Internal->Sources[index];
    }

  return 0;
}

int pqPipelineServer::GetSourceIndexFor(pqPipelineSource *source) const
{
  if(this->Internal && source)
    {
    return this->Internal->Sources.indexOf(source);
    }

  return -1;
}

bool pqPipelineServer::HasSource(pqPipelineSource *source) const
{
  return this->GetSourceIndexFor(source) != -1;
}

void pqPipelineServer::AddToSourceList(pqPipelineSource *source)
{
  if(this->Internal && source)
    {
    this->Internal->Sources.append(source);
    }
}

void pqPipelineServer::RemoveFromSourceList(pqPipelineSource *source)
{
  if(this->Internal && source)
    {
    this->Internal->Sources.removeAll(source);
    }
}

void pqPipelineServer::ClearPipelines()
{
  if(this->Internal)
    {
    // Unregister and clean up the objects in the map.
    QHash<vtkSMProxy *, pqPipelineSource *>::Iterator iter =
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

    this->Internal->Sources.clear();
    this->Internal->Objects.clear();
    this->Internal->Windows.clear();
    }
}

int pqPipelineServer::GetWindowCount() const
{
  if(this->Internal)
    {
    return this->Internal->Windows.size();
    }

  return 0;
}

QWidget *pqPipelineServer::GetWindow(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Windows.size())
    {
    return this->Internal->Windows[index];
    }

  return 0;
}

int pqPipelineServer::GetWindowIndexFor(QWidget *window) const
{
  if(this->Internal)
    {
    return this->Internal->Windows.indexOf(window);
    }

  return -1;
}

bool pqPipelineServer::HasWindow(QWidget *window) const
{
  return this->GetWindowIndexFor(window) != -1;
}

void pqPipelineServer::AddToWindowList(QWidget *window)
{
  if(this->Internal && window)
    {
    this->Internal->Windows.append(window);
    }
}

void pqPipelineServer::RemoveFromWindowList(QWidget *window)
{
  if(this->Internal && window)
    {
    this->Internal->Windows.removeAll(window);
    }
}

void pqPipelineServer::UnregisterObject(pqPipelineSource *source)
{
  /* FIXME
  if(source)
    {
    pqPipelineDisplay *display = source->getDisplay();
    if(display)
      {
      display->UnregisterDisplays();
      }

    if(!source->getProxyName().isEmpty())
      {
      pqPipelineData::instance()->unregisterProxy(source->getProxy(),
          source->getProxyName().toAscii().data());
      }
    }
    */
}


