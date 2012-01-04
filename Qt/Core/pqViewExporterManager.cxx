/*=========================================================================

   Program: ParaView
   Module:    pqViewExporterManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#include "pqViewExporterManager.h"

// Server Manager Includes.
#include "vtkSMDocumentation.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkProcessModule.h"

// Qt Includes.
#include <QDebug>
#include <QPointer>
#include <QFileInfo>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqView.h"

//-----------------------------------------------------------------------------
pqViewExporterManager::pqViewExporterManager(QObject* _parent):
  Superclass(_parent)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this, SLOT(refresh()));
  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
    SIGNAL(pluginsUpdated()),
    this, SLOT(refresh()));
  this->refresh();
}

//-----------------------------------------------------------------------------
pqViewExporterManager::~pqViewExporterManager()
{
}

//-----------------------------------------------------------------------------
void pqViewExporterManager::refresh()
{
  this->setView(this->View);
}

//-----------------------------------------------------------------------------
void pqViewExporterManager::setView(pqView* view)
{
  this->View = view;
  if (!view)
    {
    emit this->exportable(false);
    return;
    }

  // ensure the proxy-manager has instantiated the prototypes.
  if (this->View)
    {
    this->View->proxyManager()->InstantiateGroupPrototypes("exporters");
    }

  bool can_export = false;

  vtkSMProxy* proxy = view->getProxy();
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToOneGroup();
  iter->SetSkipPrototypes(false);
  iter->SetSession(proxy->GetSession());
  for (iter->Begin("exporters_prototypes");
    !can_export && !iter->IsAtEnd(); iter->Next())
    {
    vtkSMExporterProxy* prototype = vtkSMExporterProxy::SafeDownCast(
      iter->GetProxy());
    can_export = (prototype && prototype->CanExport(proxy));
    }
  iter->Delete();

  emit this->exportable(can_export);
}

//-----------------------------------------------------------------------------
QString pqViewExporterManager::getSupportedFileTypes() const
{
  QString types = "";
  if (!this->View)
    {
    return types;
    }

  QList<QString> supportedWriters;

  vtkSMProxy* proxy = this->View->getProxy();

  bool first = true;
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToOneGroup();
  iter->SetSkipPrototypes(false);
  iter->SetSession(proxy->GetSession());
  for (iter->Begin("exporters_prototypes"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMExporterProxy* prototype = vtkSMExporterProxy::SafeDownCast(
      iter->GetProxy());
    if (prototype && prototype->CanExport(proxy))
      {
      if (!first)
        {
        types += ";;";
        }
      vtkSMDocumentation* doc = prototype->GetDocumentation();
      QString help;
      if (doc && doc->GetShortHelp())
        {
        help = doc->GetShortHelp();
        }
      else
        {
        help = QString("%1 Files").arg(QString(prototype->GetFileExtension()).toUpper());
        }

      types += QString("%1 (*.%2)").arg(help).arg(prototype->GetFileExtension());
      first = false;
      }
    }
  iter->Delete();
  return types;
}

//-----------------------------------------------------------------------------
bool pqViewExporterManager::write(const QString& filename)
{
  if (!this->View)
    {
    return false;
    }

  QFileInfo info(filename);
  QString extension = info.suffix();

  vtkSMExporterProxy* exporter = 0;
  vtkSMViewProxy* proxy = vtkSMViewProxy::SafeDownCast(this->View->getProxy());
  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();

  iter->SetModeToOneGroup();
  iter->SetSkipPrototypes(false);
  iter->SetSession(proxy->GetSession());

  for (iter->Begin("exporters_prototypes"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMExporterProxy* prototype = vtkSMExporterProxy::SafeDownCast(
      iter->GetProxy());
    if (prototype && 
      prototype->CanExport(proxy) && 
      extension == prototype->GetFileExtension())
      {
      exporter = vtkSMExporterProxy::SafeDownCast(pxm->NewProxy(
          prototype->GetXMLGroup(), prototype->GetXMLName()));
      exporter->UpdateVTKObjects();
      break;
      }
    }
  iter->Delete();

  if (exporter)
    {
    pqSMAdaptor::setElementProperty(exporter->GetProperty("FileName"), 
      filename);
    exporter->UpdateVTKObjects();

    // Local calls
    exporter->SetView(proxy);
    exporter->Write();

    pqSMAdaptor::setProxyProperty(exporter->GetProperty("View"), NULL);
    exporter->UpdateVTKObjects();
    exporter->Delete();
    return true;
    }

  return false;
}
