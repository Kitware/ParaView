/*=========================================================================

   Program: ParaView
   Module:    pqRenderWindowManager.cxx

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

#include "pqActiveView.h"
#include "pqRenderWindowManager.h"

// VTK includes.
#include "QVTKWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStateLoader.h"
#include "vtkSMRenderModuleProxy.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QSet>
#include <QSignalMapper>
#include <QtDebug>
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUuid>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqMultiViewFrame.h"
#include "pqPipelineBuilder.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqXMLUtil.h"
#include "pqUndoStack.h"

#if WIN32
#include "process.h"
#define getpid _getpid
#else
#include "unistd.h"
#endif

template<class T>
uint qHash(const QPointer<T> key)
{
  return qHash((T*)key);
}
//-----------------------------------------------------------------------------
class pqRenderWindowManagerInternal
{
public:
  QPointer<pqServer> ActiveServer;
  QPointer<pqRenderViewModule> ActiveRenderModule;
  QPointer<pqMultiViewFrame> FrameBeingRemoved;

  QSet<QPointer<pqMultiViewFrame> > Frames;

  QList<QPointer<pqRenderViewModule> > PendingRenderModules;
  QSize MaxRenderWindowSize;

  pqRenderViewModule* getRenderModuleToAllocate()
    {
    foreach (pqRenderViewModule* ren, this->PendingRenderModules)
      {
      if (ren)
        {
        this->PendingRenderModules.removeAll(ren);
        return ren;
        }
      }
    pqRenderViewModule* ren  = pqPipelineBuilder::instance()->createWindow(
      this->ActiveServer);
    this->PendingRenderModules.removeAll(ren);
    return ren;
    }
};

//-----------------------------------------------------------------------------
pqRenderWindowManager::pqRenderWindowManager(QWidget* _parent/*=null*/)
  : pqMultiView(_parent)
{
  this->Internal = new pqRenderWindowManagerInternal();
  this->Internal->MaxRenderWindowSize = 
    QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (!smModel)
    {
    qDebug() << "pqServerManagerModel instance must be created before "
      <<"pqRenderWindowManager.";
    return;
    }


  QObject::connect(smModel, SIGNAL(renderModuleAdded(pqRenderViewModule*)),
    this, SLOT(onRenderModuleAdded(pqRenderViewModule*)));
  QObject::connect(smModel, SIGNAL(renderModuleRemoved(pqRenderViewModule*)),
    this, SLOT(onRenderModuleRemoved(pqRenderViewModule*)));

  QObject::connect(this, SIGNAL(frameAdded(pqMultiViewFrame*)), 
    this, SLOT(onFrameAdded(pqMultiViewFrame*)));

  QObject::connect(this, SIGNAL(frameRemoved(pqMultiViewFrame*)), 
    this, SLOT(onFrameRemoved(pqMultiViewFrame*)));

  this->init();
}

//-----------------------------------------------------------------------------
pqRenderWindowManager::~pqRenderWindowManager()
{
  // Cleanup all render modules.
  foreach (pqMultiViewFrame* frame , this->Internal->Frames)
    {
    if (frame)
      {
      this->onFrameRemoved(frame);
      }
    }
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (smModel)
    {
    QObject::disconnect(smModel, 0, this, 0);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::setActiveServer(pqServer* server)
{
  this->Internal->ActiveServer = server;
}

//-----------------------------------------------------------------------------
pqRenderViewModule* pqRenderWindowManager::getActiveRenderModule()
{
  return this->Internal->ActiveRenderModule;
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onFrameAdded(pqMultiViewFrame* frame)
{
  if (!this->Internal->ActiveServer)
    {
    return;
    }

  // Either use a previously unallocated render module, or create a new one.
  pqRenderViewModule* rm = this->Internal->getRenderModuleToAllocate();

  rm->setWindowParent(frame);
  frame->setMainWidget(rm->getWidget());
  rm->getWidget()->installEventFilter(this);
  rm->getWidget()->setMaximumSize(this->Internal->MaxRenderWindowSize);

  QSignalMapper* sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onActivate(QWidget*)));

  frame->BackButton->show();
  frame->ForwardButton->show();
  frame->MaximizeButton->show();
  frame->CloseButton->show();
  frame->SplitVerticalButton->show();
  frame->SplitHorizontalButton->show();

  QObject::connect(frame->BackButton, SIGNAL(pressed()),
    rm->getInteractionUndoStack(), SLOT(Undo()));
  QObject::connect(frame->ForwardButton, SIGNAL(pressed()),
    rm->getInteractionUndoStack(), SLOT(Redo()));
  QObject::connect(rm->getInteractionUndoStack(), SIGNAL(CanUndoChanged(bool)),
    frame->BackButton, SLOT(setEnabled(bool)));
  QObject::connect(rm->getInteractionUndoStack(), SIGNAL(CanRedoChanged(bool)),
    frame->ForwardButton, SLOT(setEnabled(bool)));


  connect(frame,SIGNAL(dragStart(pqMultiViewFrame*)),this,SLOT(frameDragStart(pqMultiViewFrame*)));
  connect(frame,SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),this,SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  connect(frame,SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),this,SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  connect(frame,SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),this,SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));


  frame->setActive(true);

  this->Internal->Frames.insert(frame);

  this->Internal->ActiveRenderModule =  rm;
  emit this->activeRenderModuleChanged(this->Internal->ActiveRenderModule);
  pqActiveView::instance().setCurrent(this->Internal->ActiveRenderModule);
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onFrameRemoved(pqMultiViewFrame* frame)
{
  // For now when a frame is removed, its render module is destroyed.
  // Later we may want to just hide it or something...for now, let's just
  // destory the render window.
  QVTKWidget* w = qobject_cast<QVTKWidget*>(frame->mainWidget());
  if (w)
    {
    this->Internal->FrameBeingRemoved = frame;
    pqRenderViewModule* rm = 
      pqServerManagerModel::instance()->getRenderModule(w);
    QObject::disconnect(frame->BackButton, 0, rm->getInteractionUndoStack(), 0);
    QObject::disconnect(frame->ForwardButton, 0, rm->getInteractionUndoStack(), 0);
    QObject::disconnect(rm->getInteractionUndoStack(), 0,frame->BackButton, 0);
    QObject::disconnect(rm->getInteractionUndoStack(), 0,frame->ForwardButton, 0);
    rm->getWidget()->removeEventFilter(this);
    pqPipelineBuilder::instance()->removeWindow(rm);
    this->Internal->FrameBeingRemoved = 0;
    }
  disconnect(frame,SIGNAL(dragStart(pqMultiViewFrame*)),this,SLOT(frameDragStart(pqMultiViewFrame*)));
  disconnect(frame,SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),this,SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  disconnect(frame,SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),this,SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  disconnect(frame,SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),this,SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  this->Internal->Frames.remove(frame);
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onRenderModuleAdded(pqRenderViewModule* rm)
{
  this->Internal->PendingRenderModules.push_back(rm);
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::allocateWindowsToRenderModules()
{
  // Try to locate a frame that has no render module in it.
QList<pqMultiViewFrame*> frames = this->SplitterFrame->findChildren<pqMultiViewFrame*>();
  foreach (pqMultiViewFrame* frame, frames)
    {
    if (this->Internal->PendingRenderModules.size() == 0)
      {
      break;
      }
    if (!frame->mainWidget())
      {
      this->onFrameAdded(frame);
      }
    }
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onRenderModuleRemoved(pqRenderViewModule* rm)
{
  pqMultiViewFrame* frame = NULL;
  if (!this->Internal->FrameBeingRemoved)
    {
    ///TODO: locate the frame for this render module and close it.
    frame = qobject_cast<pqMultiViewFrame*>(rm->getWidget()->parentWidget());
    } 
  else
    {
    frame = this->Internal->FrameBeingRemoved;
    }

  if (frame)
    {
    frame->setMainWidget(NULL);
    }
  rm->setWindowParent(NULL);
  this->Internal->PendingRenderModules.removeAll(rm);

  if (this->Internal->ActiveRenderModule == rm)
    {
    this->Internal->ActiveRenderModule = 0;
    emit this->activeRenderModuleChanged(this->Internal->ActiveRenderModule);
    pqActiveView::instance().setCurrent(this->Internal->ActiveRenderModule);
    if (this->Internal->Frames.size() > 0)
      {
      // Activate some other view, so that atleast one view is active.
      (*this->Internal->Frames.begin())->setActive(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::onActivate(QWidget* obj)
{
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(obj);
  if (!frame->active())
    {
    return;
    }
  QVTKWidget* w = qobject_cast<QVTKWidget*>(frame->mainWidget());
  pqRenderViewModule* rm = 
    pqServerManagerModel::instance()->getRenderModule(w);
  this->Internal->ActiveRenderModule = rm;
  foreach(pqMultiViewFrame* fr, this->Internal->Frames)
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }
  emit this->activeRenderModuleChanged(this->Internal->ActiveRenderModule);
  pqActiveView::instance().setCurrent(this->Internal->ActiveRenderModule);
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::setActiveView(pqGenericViewModule* view)
{
  pqRenderViewModule* ren = qobject_cast<pqRenderViewModule*>(view);
  
  if (this->Internal->ActiveRenderModule == ren)
    {
    return;
    }
  if (this->Internal->ActiveRenderModule)
    {
    pqMultiViewFrame* frame = 
      qobject_cast<pqMultiViewFrame*>(
        this->Internal->ActiveRenderModule->getWindowParent());
    frame->setActive(false);
    }
  this->Internal->ActiveRenderModule = ren;
  if (ren)
    {
    pqMultiViewFrame* frame = 
      qobject_cast<pqMultiViewFrame*>(ren->getWindowParent());
    frame->setActive(false);    
    }
}

//-----------------------------------------------------------------------------
bool pqRenderWindowManager::eventFilter(QObject* caller, QEvent* e)
{
  if(e->type() == QEvent::MouseButtonPress)
    {
    QVTKWidget* view = qobject_cast<QVTKWidget*>(caller);
    if (view)
      {
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        view->parentWidget());
      if (frame)
        {
        frame->setActive(true);
        }
      }
    }
  return QObject::eventFilter(caller, e);
}
//-----------------------------------------------------------------------------
void pqRenderWindowManager::setMaxRenderWindowSize(const QSize& win_size)
{
  this->Internal->MaxRenderWindowSize = win_size.isEmpty()?
      QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX) : win_size;
  foreach (pqMultiViewFrame* frame, this->Internal->Frames)
    {
    frame->mainWidget()->setMaximumSize(this->Internal->MaxRenderWindowSize);
    }
}

//-----------------------------------------------------------------------------
void pqRenderWindowManager::saveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* rwRoot = vtkPVXMLElement::New();
  rwRoot->SetName("RenderViewManager");
  root->AddNestedElement(rwRoot);
  rwRoot->Delete();

  // Save the window layout.
  this->pqMultiView::saveState(rwRoot);

  // Save the render module - window mapping.
  foreach(pqMultiViewFrame* frame, this->Internal->Frames)
    {
    pqMultiView::Index index = this->indexOf(frame);
    vtkPVXMLElement* frameElem = vtkPVXMLElement::New();
    frameElem->SetName("Frame");
    frameElem->AddAttribute("index", index.getString().toStdString().c_str());

    QVTKWidget* w = qobject_cast<QVTKWidget*>(frame->mainWidget());
    if (w)
      {
      pqRenderViewModule* rm = 
        pqServerManagerModel::instance()->getRenderModule(w);
      frameElem->AddAttribute("render_module", rm->getProxy()->GetSelfIDAsString());
      }
    rwRoot->AddNestedElement(frameElem);
    frameElem->Delete();
    }
}

//-----------------------------------------------------------------------------
bool pqRenderWindowManager::loadState(vtkPVXMLElement* rwRoot, 
  vtkSMStateLoader* loader)
{
  if (!rwRoot || !rwRoot->GetName() || strcmp(rwRoot->GetName(), "RenderViewManager"))
    {
    qDebug() << "Argument must be <RenderViewManager /> element.";
    return false;
    }
 
  QList<QWidget*> removed;
  this->Internal->Frames.clear();
  this->blockSignals(true);
  this->reset(removed);
  this->pqMultiView::loadState(rwRoot);
  this->blockSignals(false);
  this->Internal->PendingRenderModules.clear();
  for(unsigned int cc=0; cc < rwRoot->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* elem = rwRoot->GetNestedElement(cc);
    if (strcmp(elem->GetName(), "Frame") == 0)
      {
      QString index_string = elem->GetAttribute("index");

      pqMultiView::Index index;
      index.setFromString(index_string);
      int id = 0;
      elem->GetScalarAttribute("render_module", &id);
      vtkSMRenderModuleProxy* renModuleProxy = 
        vtkSMRenderModuleProxy::SafeDownCast(loader->NewProxy(id));
      renModuleProxy->Delete();
      pqRenderViewModule* rm =
        pqServerManagerModel::instance()->getRenderModule(renModuleProxy);
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        this->widgetOfIndex(index));
      this->onRenderModuleAdded(rm);
      this->onFrameAdded(frame);
      }
    }
  foreach(QWidget* wid, removed)
    {
    pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(wid);
    if (frame)
      {
      // If the user has more windows open than the ones specified in the state,
      // the view no longer lays out those extra render modules. We mark all
      // these render modules as "pending". As user splits new views, he will
      // see these old render modules that got hidden from view.
      QVTKWidget* _window = qobject_cast<QVTKWidget*>(frame->mainWidget());
      if (_window)
        {
        pqRenderViewModule* ren = pqServerManagerModel::instance()->getRenderModule(_window);
        if (ren)
          {
          this->Internal->PendingRenderModules.push_back(ren);
          ren->setWindowParent(0);
          frame->setMainWidget(0);
          }
        }
      }
    delete wid;
    }
  return true;
}
void pqRenderWindowManager::frameDragStart(pqMultiViewFrame* frame)
{
  QPixmap pixmap(":/pqWidgets/Icons/pqWindow16.png");

  QByteArray output;
  QDataStream dataStream(&output, QIODevice::WriteOnly);
  dataStream<<frame->uniqueID();

  QString mimeType = QString("application/paraview3/%1").arg(getpid());

  QMimeData *mimeData = new QMimeData;
  mimeData->setData(mimeType, output);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
  drag->setPixmap(pixmap);

  drag->start();
}
void pqRenderWindowManager::frameDragEnter(pqMultiViewFrame*,
                                           QDragEnterEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if(e->mimeData()->hasFormat(mimeType))
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}
void pqRenderWindowManager::frameDragMove(pqMultiViewFrame*,
                                          QDragMoveEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if(e->mimeData()->hasFormat(mimeType))
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}
void pqRenderWindowManager::frameDrop(pqMultiViewFrame* acceptingFrame,
                                      QDropEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if (e->mimeData()->hasFormat(mimeType))
    {
    QByteArray input= e->mimeData()->data(mimeType);
    QDataStream dataStream(&input, QIODevice::ReadOnly);

    QUuid uniqueID;
    dataStream>>uniqueID;

    pqMultiViewFrame* originatingFrame=NULL;
    pqMultiViewFrame* f;
    foreach(f,this->Internal->Frames)
      {
      if(f->uniqueID()==uniqueID)
        {
        originatingFrame=f;
        break;
        }
      }

    if(originatingFrame)
      {
      this->hide(); 
      //Switch the originalFrame with the frame;

      Index originatingIndex=this->indexOf(originatingFrame);
      Index acceptingIndex=this->indexOf(acceptingFrame);
      pqMultiViewFrame *tempFrame= new pqMultiViewFrame;

      this->replaceView(originatingIndex,tempFrame);
      this->replaceView(acceptingIndex,originatingFrame);
      originatingIndex=this->indexOf(tempFrame);
      this->replaceView(originatingIndex,acceptingFrame);

      delete tempFrame;
      
      this->show();


      }
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

