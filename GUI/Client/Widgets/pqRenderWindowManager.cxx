/*=========================================================================

   Program:   ParaQ
   Module:    pqRenderWindowManager.cxx

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

#include "pqRenderWindowManager.h"

// ParaView includes.
#include "QVTKWidget.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QSignalMapper>
#include <QtDebug>

// ParaQ includes.
#include "pqMultiViewFrame.h"
#include "pqPipelineBuilder.h"
#include "pqRenderModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"

//-----------------------------------------------------------------------------
class pqRenderWindowManagerInternal
{
public:
  //QPointer<pqServer> ActiveServer;
  QPointer<pqRenderModule> ActiveRenderModule;
  QPointer<pqMultiViewFrame> FrameAdded;
};

//-----------------------------------------------------------------------------
pqRenderWindowManager::pqRenderWindowManager(QObject* _parent/*=null*/)
  : QObject(_parent)
{
  this->Internal = new pqRenderWindowManagerInternal();
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (!smModel)
    {
    qDebug() << "pqServerManagerModel instance must be created before "
      <<"pqRenderWindowManager.";
    return;
    }

  QObject::connect(smModel, SIGNAL(renderModuleAdded(pqRenderModule*)),
    this, SLOT(onRenderModuleAdded(pqRenderModule*)));
  QObject::connect(smModel, SIGNAL(renderModuleRemoved(pqRenderModule*)),
    this, SLOT(onRenderModuleRemoved(pqRenderModule*)));
}

//-----------------------------------------------------------------------------
pqRenderWindowManager::~pqRenderWindowManager()
{
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (smModel)
    {
    QObject::disconnect(smModel, 0, this, 0);
    }
  delete this->Internal;
}

/*//-----------------------------------------------------------------------------
void pqRenderWindowManager::setActiveServer(pqServer* server)
{
  this->Internal->ActiveServer = server;
}
*/
//-----------------------------------------------------------------------------
pqRenderModule* pqRenderWindowManager::getActiveRenderModule()
{
  return this->Internal->ActiveRenderModule;
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onFrameAdded(pqMultiViewFrame* frame)
{
  if (!pqApplicationCore::instance()->getActiveServer())
    {
    return;
    }

  this->Internal->FrameAdded = frame;
  pqRenderModule* rm =   
    pqPipelineBuilder::instance()->createWindow(pqApplicationCore::instance()->getActiveServer());
  this->Internal->ActiveRenderModule =  rm;
  emit this->activeRenderModuleChanged(this->Internal->ActiveRenderModule);


  QSignalMapper* sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onActivate(QWidget*)));

  frame->setActive(true);
  this->Internal->FrameAdded = 0;
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onFrameRemoved(pqMultiViewFrame* frame)
{
  // For now when a frame is removed, its render module is destroyed.
  // Later we may want to just hide it or something...for now, let's just
  // destory the render window.
  QVTKWidget* widget =qobject_cast<QVTKWidget*>(frame->mainWidget());
  if (widget)
    {
    this->Internal->FrameAdded = frame;
    pqRenderModule* rm = 
      pqServerManagerModel::instance()->getRenderModule(widget);
    pqPipelineBuilder::instance()->removeWindow(rm);
    this->Internal->FrameAdded = 0;
    }
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onRenderModuleAdded(pqRenderModule* rm)
{
  if (!this->Internal->FrameAdded)
    {
    qDebug() << "RM creation not initiated by GUI. This case it not handled yet.";
    return;
    // SHould split the view and create a new window for this render module.
    }
  rm->setWindowParent(this->Internal->FrameAdded);
  this->Internal->FrameAdded->setMainWidget(rm->getWidget());
  this->Internal->FrameAdded = 0; // since the frame cannot be reused
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onRenderModuleRemoved(pqRenderModule* rm)
{
  pqMultiViewFrame* frame = NULL;
  if (!this->Internal->FrameAdded)
    {
    ///TODO: locate the frame for this render module and close it.
    frame = qobject_cast<pqMultiViewFrame*>(rm->getWidget()->parentWidget());
    } 
  else
    {
    frame = this->Internal->FrameAdded;
    }

  if (frame)
    {
    frame->setMainWidget(NULL);
    }

  if (this->Internal->ActiveRenderModule == rm)
    {
    this->Internal->ActiveRenderModule = 0;
    emit this->activeRenderModuleChanged(this->Internal->ActiveRenderModule);

    }
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onActivate(QWidget* obj)
{
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(obj);
  QVTKWidget* widget =qobject_cast<QVTKWidget*>(frame->mainWidget());
  pqRenderModule* rm = 
    pqServerManagerModel::instance()->getRenderModule(widget);
  this->Internal->ActiveRenderModule = rm;
  emit this->activeRenderModuleChanged(this->Internal->ActiveRenderModule);

  
}
