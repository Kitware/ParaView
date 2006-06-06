/*=========================================================================

   Program:   ParaQ
   Module:    pq3DWidgetFactory.cxx

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
#include "pq3DWidgetFactory.h"

// ParaView includes.
#include "vtkSMNew3DWidgetProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"

// Qt Includes.
#include <QList>
#include <QtDebug>

// ParaQ includes.
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqServerManagerObserver.h"
#include "pqServer.h"

class pq3DWidgetFactoryInternal
{
public:
  typedef QList<vtkSmartPointer<vtkSMNew3DWidgetProxy> > ListOfWidgetProxies;
  ListOfWidgetProxies Widgets;
  ListOfWidgetProxies WidgetsInUse;
};

//-----------------------------------------------------------------------------
pq3DWidgetFactory::pq3DWidgetFactory(QObject* _parent/*=null*/)
: QObject(_parent)
{
  this->Internal = new pq3DWidgetFactoryInternal();
  QObject::connect(pqApplicationCore::instance()->getPipelineData(),
    SIGNAL(proxyUnRegistered(QString, QString, vtkSMProxy*)), this, 
    SLOT(proxyUnRegistered(QString, QString, vtkSMProxy*)));
}

//-----------------------------------------------------------------------------
pq3DWidgetFactory::~pq3DWidgetFactory()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMNew3DWidgetProxy* pq3DWidgetFactory::get3DWidget(const QString& name,
    pqServer* server)
{
  pq3DWidgetFactoryInternal::ListOfWidgetProxies::iterator iter =
    this->Internal->Widgets.begin();
  for (; iter != this->Internal->Widgets.end(); iter++)
    {
    vtkSMNew3DWidgetProxy* proxy = iter->GetPointer();
    if (proxy && proxy->GetConnectionID() == server->GetConnectionID()
      && name == proxy->GetXMLName())
      {
      this->Internal->WidgetsInUse.push_back(proxy);
      this->Internal->Widgets.erase(iter);
      return proxy;
      }
    }

  pqPipelineBuilder* builder = 
    pqApplicationCore::instance()->getPipelineBuilder();

  vtkSMNew3DWidgetProxy* proxy = vtkSMNew3DWidgetProxy::SafeDownCast(
    builder->createProxy("displays", name.toStdString().c_str(), 
      "3d_widgets", server, false));
  if (!proxy)
    {
    qDebug() << "Could not create the 3D widget with name: " << name;
    return NULL;
    }
  this->Internal->WidgetsInUse.push_back(proxy);
  return proxy;
}

//-----------------------------------------------------------------------------
void pq3DWidgetFactory::free3DWidget(vtkSMNew3DWidgetProxy* widget)
{
  pq3DWidgetFactoryInternal::ListOfWidgetProxies::iterator iter =
    this->Internal->WidgetsInUse.begin();
  for (; iter != this->Internal->WidgetsInUse.end(); iter++)
    {
    vtkSMNew3DWidgetProxy* proxy = iter->GetPointer();
    if (proxy == widget)
      {
      this->Internal->Widgets.push_back(proxy);
      this->Internal->WidgetsInUse.erase(iter);
      return;
      }
    }
  // qDebug() << "free3DWidget called on a free widget on a widget not managed"
  // " by this class.";
}

//-----------------------------------------------------------------------------
void pq3DWidgetFactory::proxyUnRegistered(QString group, 
  QString vtkNotUsed(name), vtkSMProxy* proxy)
{
  vtkSMNew3DWidgetProxy* widget;
  if (group != "3d_widgets" || 
    (widget = vtkSMNew3DWidgetProxy::SafeDownCast(proxy)) == 0)
    {
    return;
    }
  // Check if the unregistered proxy is the one managed by this class.
  pq3DWidgetFactoryInternal::ListOfWidgetProxies::iterator iter;
  for (iter = this->Internal->WidgetsInUse.begin(); 
    iter != this->Internal->WidgetsInUse.end(); iter++)
    {
    if (iter->GetPointer() == widget)
      {
      this->Internal->WidgetsInUse.erase(iter);
      break;
      }
    }

  for (iter = this->Internal->Widgets.begin();
    iter != this->Internal->Widgets.end(); ++iter)
    {
    if (iter->GetPointer() == widget)
      {
      this->Internal->Widgets.erase(iter);
      break;
      }
    }
}
