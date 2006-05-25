/*=========================================================================

   Program:   ParaQ
   Module:    pqApplicationCore.cxx

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

========================================================================*/
#include "pqApplicationCore.h"

// ParaView includes.
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"

// Qt includes.
#include <QPointer>
#include <QtDebug>


// ParaQ includes.
#include "pq3DWidgetFactory.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqRenderWindowManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqApplicationCoreInternal
{
public:
  pqPipelineData* PipelineData;
  pqServerManagerModel* ServerManagerModel;
  pqUndoStack* UndoStack;
  pqPipelineBuilder* PipelineBuilder;
  pqRenderWindowManager* RenderWindowManager;
  pq3DWidgetFactory* WidgetFactory;

  QPointer<pqPipelineSource> ActiveSource;
  QPointer<pqServer> ActiveServer;

  QList< QPointer<pqPipelineSource> > SourcesSansDisplays;
};


//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::Instance = 0;

//-----------------------------------------------------------------------------
pqApplicationCore* pqApplicationCore::instance()
{
  return pqApplicationCore::Instance;
}

//-----------------------------------------------------------------------------
pqApplicationCore::pqApplicationCore(QObject* p/*=null*/)
  : QObject(p)
{
  this->Internal = new pqApplicationCoreInternal();

  // *  Create pqPipelineData first. This is the vtkSMProxyManager observer.
  this->Internal->PipelineData = new pqPipelineData(this);

  // *  Create pqServerManagerModel.
  //    This is the representation builder for the ServerManager state.
  this->Internal->ServerManagerModel = new pqServerManagerModel(this);

  // *  Make signal-slot connections between PipelineData and ServerManagerModel.
  this->connect(this->Internal->PipelineData, this->Internal->ServerManagerModel);


  // *  Create the Undo/Redo stack.
  this->Internal->UndoStack = new pqUndoStack(this);

  // *  Create the pqPipelineBuilder. This is used to create pipeline objects.
  this->Internal->PipelineBuilder = new pqPipelineBuilder(this);
  this->Internal->PipelineBuilder->setUndoStack(this->Internal->UndoStack);

  // *  Create the pqRenderWindowManager. 
  this->Internal->RenderWindowManager = new pqRenderWindowManager(this);

  if (!pqApplicationCore::Instance)
    {
    pqApplicationCore::Instance = this;
    }

  // We catch sourceRemoved signal to detect when the ActiveSource
  // is deleted, if so we need to change the ActiveSource.
  QObject::connect(this->Internal->ServerManagerModel,
    SIGNAL(sourceRemoved(pqPipelineSource*)),
    this, SLOT(sourceRemoved(pqPipelineSource*)));

  // * Create the pq3DWidgetFactory.
  this->Internal->WidgetFactory = new pq3DWidgetFactory(this);
}

//-----------------------------------------------------------------------------
pqApplicationCore::~pqApplicationCore()
{
  if (pqApplicationCore::Instance == this)
    {
    pqApplicationCore::Instance = 0;
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::connect(pqPipelineData* pdata, 
  pqServerManagerModel* smModel)
{
  QObject::connect(pdata, SIGNAL(sourceRegistered(QString, vtkSMProxy*)),
    smModel, SLOT(onAddSource(QString, vtkSMProxy*)));
  QObject::connect(pdata, SIGNAL(sourceUnRegistered(vtkSMProxy*)),
    smModel, SLOT(onRemoveSource(vtkSMProxy*)));
  QObject::connect(pdata, SIGNAL(connectionCreated(vtkIdType)),
    smModel, SLOT(onAddServer(vtkIdType)));
  QObject::connect(pdata, SIGNAL(connectionClosed(vtkIdType)),
    smModel, SLOT(onRemoveServer(vtkIdType)));
  QObject::connect(pdata, SIGNAL(renderModuleRegistered(QString, 
        vtkSMRenderModuleProxy*)),
    smModel, SLOT(onAddRenderModule(QString, vtkSMRenderModuleProxy*)));
  QObject::connect(pdata, SIGNAL(renderModuleUnRegistered(vtkSMRenderModuleProxy*)),
    smModel, SLOT(onRemoveRenderModule(vtkSMRenderModuleProxy*)));
  QObject::connect(pdata, 
    SIGNAL(displayRegistered(QString, vtkSMProxy*)),
    smModel, SLOT(onAddDisplay(QString, vtkSMProxy*)));
  QObject::connect(pdata, SIGNAL(displayUnRegistered(vtkSMProxy*)),
    smModel, SLOT(onRemoveDisplay(vtkSMProxy*)));
      
}

//-----------------------------------------------------------------------------
pqPipelineData* pqApplicationCore::getPipelineData()
{
  return this->Internal->PipelineData;
}

//-----------------------------------------------------------------------------
pqServerManagerModel* pqApplicationCore::getServerManagerModel()
{
  return this->Internal->ServerManagerModel;
}

//-----------------------------------------------------------------------------
pqUndoStack* pqApplicationCore::getUndoStack()
{
  return this->Internal->UndoStack;
}

//-----------------------------------------------------------------------------
pqPipelineBuilder* pqApplicationCore::getPipelineBuilder()
{
  return this->Internal->PipelineBuilder;
}

//-----------------------------------------------------------------------------
pqRenderWindowManager* pqApplicationCore::getRenderWindowManager()
{
  return this->Internal->RenderWindowManager;
}

//-----------------------------------------------------------------------------
pq3DWidgetFactory* pqApplicationCore::get3DWidgetFactory()
{
  return this->Internal->WidgetFactory;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setActiveSource(pqPipelineSource* src)
{
  if (this->Internal->ActiveSource == src)
    {
    return;
    }

  this->Internal->ActiveSource = src;
  emit this->activeSourceChanged(src);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::setActiveServer(pqServer* server)
{
  if (this->Internal->ActiveServer == server)
    {
    return;
    }
  this->Internal->ActiveServer = server;
  this->Internal->RenderWindowManager->setActiveServer(server);
  emit this->activeServerChanged(server);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqApplicationCore::getActiveSource()
{
  return this->Internal->ActiveSource;
}

//-----------------------------------------------------------------------------
pqServer* pqApplicationCore::getActiveServer()
{
  return this->Internal->ActiveServer;
}

//-----------------------------------------------------------------------------
pqRenderModule* pqApplicationCore::getActiveRenderModule()
{
  return this->Internal->RenderWindowManager->getActiveRenderModule();
}

//-----------------------------------------------------------------------------
int pqApplicationCore::getNumberOfSourcesPendingDisplays()
{
  return this->Internal->SourcesSansDisplays.size();
}

//-----------------------------------------------------------------------------
void pqApplicationCore::sourceRemoved(pqPipelineSource* source)
{
  if (source == this->getActiveSource())
    {
    pqServer* server =this->getActiveServer();
    this->setActiveSource(NULL);
    this->Internal->ActiveServer = 0;
    this->setActiveServer(server);
    }

  if (this->Internal->SourcesSansDisplays.contains(source))
    {
    this->Internal->SourcesSansDisplays.removeAll(source);
    if (this->Internal->SourcesSansDisplays.size() == 0)
      {
      emit this->pendingDisplays(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqApplicationCore::render()
{
  pqRenderModule* renModule = this->getActiveRenderModule();
  if (renModule)
    {
    renModule->render();
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqApplicationCore::createSourceOnActiveServer(
  const QString& xmlname)
{
  if (!this->Internal->ActiveServer)
    {
    qDebug() << "No server currently active. "
      << "Cannot createSourceOnActiveServer.";
    return 0;
    }

  pqPipelineSource* source = this->Internal->PipelineBuilder->createSource(
    "sources", xmlname.toStdString().c_str(), 
    this->Internal->ActiveServer, NULL);

  if (source)
    {
    this->Internal->SourcesSansDisplays.push_back(source);
    emit this->pendingDisplays(true);
    this->setActiveSource(source);
    }

  return source;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqApplicationCore::createFilterForActiveSource(
  const QString& xmlname)
{
  if (!this->Internal->ActiveSource)
    {
    qDebug() << "No source/filter active. Cannot createFilterForActiveSource.";
    return 0;
    }

  pqPipelineSource* filter = this->Internal->PipelineBuilder->createSource(
    "filters", xmlname.toStdString().c_str(), 
    this->Internal->ActiveSource->getServer(), NULL);

  if (filter)
    {
    this->Internal->PipelineBuilder->addConnection(
      this->Internal->ActiveSource, filter);

    // As a special-case, set a default implicit function for new Clip filters
    if(xmlname == "Clip")
      {
      vtkSMProxy* const plane = this->Internal->PipelineBuilder->createProxy(
        "implicit_functions", "Plane", "implicit_functions", 
        this->Internal->ActiveSource->getServer());
      
      this->Internal->UndoStack->BeginOrContinueUndoSet("Set ClipConnection");
      if(vtkSMProxyProperty* const clip_function = 
        vtkSMProxyProperty::SafeDownCast(
          filter->getProxy()->GetProperty("ClipFunction")))
        {
        clip_function->AddProxy(plane);
        }
      this->Internal->UndoStack->PauseUndoSet();
      }

    // As a special-case, set a default implicit function for new Cut filters
    if(xmlname == "Cut")
      {
      vtkSMProxy* const plane = this->Internal->PipelineBuilder->createProxy(
        "implicit_functions", "Plane", "implicit_functions", 
        this->Internal->ActiveSource->getServer());
      
      this->Internal->UndoStack->BeginOrContinueUndoSet("Set CutConnection");
      
      if(vtkSMProxyProperty* const cut_function = 
        vtkSMProxyProperty::SafeDownCast(
          filter->getProxy()->GetProperty("CutFunction")))
        {
        cut_function->AddProxy(plane);
        }
        
      if(vtkSMDoubleVectorProperty* const contours =
        vtkSMDoubleVectorProperty::SafeDownCast(
          filter->getProxy()->GetProperty("ContourValues")))
        {
        contours->SetNumberOfElements(1);
        contours->SetElement(0, 0.0);
        }
        
      this->Internal->UndoStack->PauseUndoSet();
      }
    }

  if (filter)
    {
    this->Internal->SourcesSansDisplays.push_back(filter);
    emit this->pendingDisplays(true);
    this->setActiveSource(filter);
    }
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqApplicationCore::createCompoundSource(
  const QString& name)
{
  // For starters all compoind proxies need an input..
  if (!this->Internal->ActiveSource)
    {
    qDebug() << "No source/filter active. Cannot createFilterForActiveSource.";
    return 0;
    }

  pqPipelineSource* filter = this->Internal->PipelineBuilder->createSource(
    NULL, name.toStdString().c_str(), 
    this->Internal->ActiveSource->getServer(), NULL);

  if (filter)
    {
    this->Internal->PipelineBuilder->addConnection(
      this->Internal->ActiveSource, filter);

    this->Internal->SourcesSansDisplays.push_back(filter);
    emit this->pendingDisplays(true);
    this->setActiveSource(filter);
    }

  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqApplicationCore::createReaderOnActiveServer( 
  const QString& filename, const QString& readerName)
{
  if (!this->Internal->ActiveServer)
    {
    qDebug() << "No active server. Cannot create reader.";
    return 0;
    }

  pqPipelineSource* reader = this->Internal->PipelineBuilder->createSource(
    "sources", readerName.toStdString().c_str(), 
    this->Internal->ActiveServer, NULL);

  if (!reader)
    {
    return NULL;
    }

  this->Internal->UndoStack->BeginOrContinueUndoSet("Set Filenames");

  vtkSMProxy* proxy = reader->getProxy();
  pqSMAdaptor::setElementProperty(proxy, proxy->GetProperty("FileName"), 
    filename);
  pqSMAdaptor::setElementProperty(proxy, proxy->GetProperty("FilePrefix"),
    filename);
  pqSMAdaptor::setElementProperty(proxy, proxy->GetProperty("FilePattern"),
    filename);
  proxy->UpdateVTKObjects();

  this->Internal->UndoStack->PauseUndoSet();

  this->Internal->SourcesSansDisplays.push_back(reader);
  emit this->pendingDisplays(true);

  this->setActiveSource(reader);
  return reader;
}

//-----------------------------------------------------------------------------
void pqApplicationCore::removeActiveSource()
{
  pqPipelineSource* source = this->getActiveSource();
  if (!source)
    {
    qDebug() << "No active source to remove.";
    return;
    }
  this->removeSource(source);
}

//-----------------------------------------------------------------------------
void pqApplicationCore::removeSource(pqPipelineSource* source)
{
  if (!source)
    {
    qDebug() << "No source to remove.";
    return;
    }
  if (source->getNumberOfConsumers())
    {
    qDebug() << "Active source has consumers, cannot delete";
    return;
    }
 
  this->getPipelineBuilder()->remove(source);

  // Since pqPipelineBuilder is never going to call EndUndoSet(), we must call 
  // it explicitly here.
  this->getUndoStack()->EndUndoSet();
  this->getActiveRenderModule()->render();
}


//-----------------------------------------------------------------------------
void pqApplicationCore::createPendingDisplays()
{
  foreach (pqPipelineSource* source, this->Internal->SourcesSansDisplays)
    {
    if (!source)
      {
      continue;
      }
    this->Internal->PipelineBuilder->createDisplayProxy(source,
      this->getActiveRenderModule());
    this->getActiveRenderModule()->resetCamera();
    this->getActiveRenderModule()->render();
    if (this->getServerManagerModel()->getNumberOfSources() == 1)
      {
      this->getActiveRenderModule()->resetCamera();
      }
    }

  this->Internal->SourcesSansDisplays.clear();
  emit this->pendingDisplays(false);
}
