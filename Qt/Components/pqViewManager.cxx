/*=========================================================================

   Program: ParaView
   Module:    pqViewManager.cxx

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
#include "pqViewManager.h"

// VTK includes.
#include "QVTKWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMIntVectorProperty.h"

// Qt includes.
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QMap>
#include <QMimeData>
#include <QPointer>
#include <QSet>
#include <QSignalMapper>
#include <QtDebug>
#include <QUuid>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqMultiViewFrame.h"
#include "pqPipelineBuilder.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqXMLUtil.h"

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
class pqViewManager::pqInternals 
{
public:
  QPointer<pqServer> ActiveServer;
  QPointer<pqGenericViewModule> ActiveViewModule;
  QPointer<pqMultiViewFrame> FrameBeingRemoved;

  typedef QMap<pqMultiViewFrame*, QPointer<pqGenericViewModule> > FrameMapType;
  FrameMapType Frames;

  QList<QPointer<pqMultiViewFrame> > PendingFrames;

  QSize MaxWindowSize;

  bool DontCreateDeleteViewsModules;
  pqGenericViewModule* getViewModuleToAllocate()
    {
    if (!this->ActiveServer || this->DontCreateDeleteViewsModules)
      {
      return NULL;
      }

    // Create a new module (for now we'll create a render module always.
    pqGenericViewModule* ren  = pqPipelineBuilder::instance()->createWindow(
      this->ActiveServer);
    return ren;
    }
};

//-----------------------------------------------------------------------------
pqViewManager::pqViewManager(QWidget* _parent/*=null*/)
  : pqMultiView(_parent)
{
  this->Internal = new pqInternals();
  this->Internal->DontCreateDeleteViewsModules = false;
  this->Internal->MaxWindowSize = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (!smModel)
    {
    qDebug() << "pqServerManagerModel instance must be created before "
      <<"pqViewManager.";
    return;
    }

  // We need to know when new view modules are added.
  QObject::connect(smModel, SIGNAL(viewModuleAdded(pqGenericViewModule*)),
    this, SLOT(onViewModuleAdded(pqGenericViewModule*)));
  QObject::connect(smModel, SIGNAL(viewModuleRemoved(pqGenericViewModule*)),
    this, SLOT(onViewModuleRemoved(pqGenericViewModule*)));

  // Record creation/removal of frames.
  QObject::connect(this, SIGNAL(frameAdded(pqMultiViewFrame*)), 
    this, SLOT(onFrameAdded(pqMultiViewFrame*)));
  QObject::connect(this, SIGNAL(frameRemoved(pqMultiViewFrame*)), 
    this, SLOT(onFrameRemoved(pqMultiViewFrame*)));

  // Creates the default empty frame.
  this->init();
}

//-----------------------------------------------------------------------------
pqViewManager::~pqViewManager()
{
  // Cleanup all render modules.
  foreach (pqMultiViewFrame* frame , this->Internal->Frames.keys())
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
void pqViewManager::setActiveServer(pqServer* server)
{
  this->Internal->ActiveServer = server;
}

//-----------------------------------------------------------------------------
pqGenericViewModule* pqViewManager::getActiveViewModule() const
{
  return this->Internal->ActiveViewModule;
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameAdded(pqMultiViewFrame* frame)
{
  // We connect drag-drop signals event for empty frames.
  QObject::connect(frame, SIGNAL(dragStart(pqMultiViewFrame*)),
    this, SLOT(frameDragStart(pqMultiViewFrame*)));
  QObject::connect(frame, SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),
    this, SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  QObject::connect(frame, SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),
    this, SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  QObject::connect(frame, SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),
    this, SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  frame->MaximizeButton->show();
  frame->CloseButton->show();
  frame->SplitVerticalButton->show();
  frame->SplitHorizontalButton->show();

  frame->MaximizeButton->setEnabled(false);
  frame->SplitVerticalButton->setEnabled(false);
  frame->SplitHorizontalButton->setEnabled(false);

  QSignalMapper* sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onActivate(QWidget*)));

  this->Internal->PendingFrames.removeAll(frame);
  this->Internal->PendingFrames.push_front(frame);

  if (this->Internal->DontCreateDeleteViewsModules)
    {
    return;
    }

  // We push this new frame at the head of pending frames and
  // ask for a suitable view. If a new view module is created,
  // it will automatically be added to  the first frame in the
  // PendingFrames.

  // Either use a previously unallocated render module, or create a new one.
  pqGenericViewModule* view = this->Internal->getViewModuleToAllocate();
  (void)view;
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameRemoved(pqMultiViewFrame* frame)
{
  QObject::disconnect(frame, SIGNAL(dragStart(pqMultiViewFrame*)),
    this, SLOT(frameDragStart(pqMultiViewFrame*)));
  QObject::disconnect(frame, SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),
    this, SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  QObject::disconnect(frame, SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),
    this, SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  QObject::disconnect(frame, SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),
    this, SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  this->Internal->PendingFrames.removeAll(frame);
  if (!this->Internal->Frames.contains(frame))
    {
    // A frame with no view module has been removed. 
    return;
    }

  // When a frame is removed, its render module is destroyed.
  pqGenericViewModule* view = this->Internal->Frames.take(frame);
  this->disconnect(frame, view);

  this->Internal->PendingFrames.removeAll(frame);

  // Generally, we destroy the view module when the frame goes away,
  // unless told otherwise.
  if (this->Internal->DontCreateDeleteViewsModules)
    {
    return;
    }

  // When a frame is removed, the contained view is also destroyed.
  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(view);
  if (rm)
    {
    pqPipelineBuilder::instance()->removeWindow(rm);
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::connect(pqMultiViewFrame* frame, pqGenericViewModule* view)
{
  if (!frame || !view)
    {
    return;
    }
  this->Internal->PendingFrames.removeAll(frame);

  view->setWindowParent(frame);
  frame->setMainWidget(view->getWidget());
  view->getWidget()->installEventFilter(this);
  view->getWidget()->setMaximumSize(this->Internal->MaxWindowSize);

  if (view->supportsUndo())
    {
    // Setup undo/redo connections if the view module
    // supports interaction undo.
    frame->BackButton->show();
    frame->ForwardButton->show();

    pqUndoStack* stack = view->getInteractionUndoStack();
    QObject::connect(frame->BackButton, SIGNAL(pressed()), 
      stack, SLOT(Undo()));
    QObject::connect(frame->ForwardButton, SIGNAL(pressed()),
      stack, SLOT(Redo()));
    QObject::connect(stack, SIGNAL(CanUndoChanged(bool)),
      frame->BackButton, SLOT(setEnabled(bool)));
    QObject::connect(stack, SIGNAL(CanRedoChanged(bool)),
      frame->ForwardButton, SLOT(setEnabled(bool)));
    }
  else
    {
    frame->BackButton->hide();
    frame->ForwardButton->hide();
    }
  frame->MaximizeButton->setEnabled(true);
  frame->SplitVerticalButton->setEnabled(true);
  frame->SplitHorizontalButton->setEnabled(true);

  this->Internal->Frames.insert(frame, view);
}

//-----------------------------------------------------------------------------
void pqViewManager::disconnect(pqMultiViewFrame* frame, pqGenericViewModule* view)
{
  if (!frame || !view)
    {
    return;
    }

  this->Internal->Frames.remove(frame);

  view->setWindowParent(NULL);
  frame->setMainWidget(NULL);
  view->getWidget()->removeEventFilter(this);

  pqUndoStack* stack = view->getInteractionUndoStack();
  if (view->supportsUndo() && stack)
    {
    QObject::disconnect(frame->BackButton, 0, stack, 0);
    QObject::disconnect(frame->ForwardButton, 0, stack, 0);
    QObject::disconnect(stack, 0,frame->BackButton, 0);
    QObject::disconnect(stack, 0,frame->ForwardButton, 0);
    }
  frame->BackButton->hide();
  frame->ForwardButton->hide();
  frame->MaximizeButton->setEnabled(false);
  frame->SplitVerticalButton->setEnabled(false);
  frame->SplitHorizontalButton->setEnabled(false);

  this->Internal->PendingFrames.push_back(frame);
}

//-----------------------------------------------------------------------------
void pqViewManager::onViewModuleAdded(pqGenericViewModule* view)
{
  this->assignFrame(view);
}

//-----------------------------------------------------------------------------
void pqViewManager::assignFrame(pqGenericViewModule* view)
{
  pqMultiViewFrame* frame = 0;
  if (this->Internal->PendingFrames.size() == 0)
    {
    // Create a new frame.
   
    // Locate frame to split.
    // If there is an active view, use it.
    pqMultiViewFrame* oldFrame = 0;
    if (this->Internal->ActiveViewModule)
      {
      oldFrame = this->getFrame(this->Internal->ActiveViewModule);
      }
    else if (this->Internal->Frames.size() > 0)
      {
      oldFrame = this->Internal->Frames.begin().key();
      }
    else
      {
      // There are no pending frames and not used frames, how
      // can that be? Hence, we are use it will happen,
      // atleast flag an error!
      qCritical() << "Internal state of frames has got messed up!";
      return;
      }
  
    this->Internal->DontCreateDeleteViewsModules = true;
    QSize cur_size = oldFrame->size();
    if (cur_size.width() > cur_size.height())
      {
      frame = this->splitWidgetHorizontal(oldFrame);
      }
    else
      {
      frame = this->splitWidgetVertical(oldFrame);
      }
    this->Internal->DontCreateDeleteViewsModules = false;
    }
  else
    {
    // Use existing available frame.
    frame = this->Internal->PendingFrames.first();
    this->Internal->PendingFrames.pop_front();
    }

  if (frame)
    {
    this->connect(frame, view);
    frame->setActive(true);
    }
}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqViewManager::getFrame(pqGenericViewModule* view) const
{
  return qobject_cast<pqMultiViewFrame*>(view->getWindowParent());
}

//-----------------------------------------------------------------------------
void pqViewManager::onViewModuleRemoved(pqGenericViewModule* view)
{
  pqMultiViewFrame* frame = this->getFrame(view);
  if (frame)
    {
    this->disconnect(frame, view);
    // close the frame for this view as well.
    this->removeWidget(frame);
    }

  if (this->Internal->ActiveViewModule == view)
    {
    if (this->Internal->Frames.size() > 0)
      {
      // Activate some other view, so that atleast one view is active.
      this->Internal->Frames.begin().key()->setActive(true);
      }
    else
      {
      this->onActivate(NULL);
      }
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onActivate(QWidget* obj)
{
  if (!obj)
    {
    this->Internal->ActiveViewModule = 0;
    emit this->activeViewModuleChanged(this->Internal->ActiveViewModule);
    return; 
    }

  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(obj);
  if (!frame->active())
    {
    return;
    }

  pqGenericViewModule* view = this->Internal->Frames.value(frame);
  // If the frame does not have a view, view==NULL.
  this->Internal->ActiveViewModule = view;

  // Make sure no other frame is active.
  foreach (pqMultiViewFrame* fr, this->Internal->Frames.keys())
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }
  foreach (pqMultiViewFrame* fr, this->Internal->PendingFrames)
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }

  emit this->activeViewModuleChanged(this->Internal->ActiveViewModule);
}

//-----------------------------------------------------------------------------
bool pqViewManager::eventFilter(QObject* caller, QEvent* e)
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
  else if (e->type() == QEvent::FocusIn)
    {
    QWidget* wdg = qobject_cast<QWidget*>(caller);
    if (wdg)
      {
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        wdg->parentWidget());
      if (frame)
        {
        frame->setActive(true);
        }
      }
    }
  else if (e->type() == QEvent::Resize)
    {
    // Update WindowPosition and GUISize properties on all view modules.
    this->updateViewModulePositions(); 
    }
  return QObject::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqViewManager::setMaxViewWindowSize(const QSize& win_size)
{
  this->Internal->MaxWindowSize = win_size.isEmpty()?
      QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX) : win_size;
  foreach (pqMultiViewFrame* frame, this->Internal->Frames.keys())
    {
    frame->mainWidget()->setMaximumSize(this->Internal->MaxWindowSize);
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::updateViewModulePositions()
{
  // find a rectangle that bounds all views
  QRect totalBounds;
  
  foreach(pqGenericViewModule* view, this->Internal->Frames)
    {
    QRect bounds = view->getWidget()->rect();
    bounds.moveTo(view->getWidget()->mapToGlobal(QPoint(0,0)));
    totalBounds |= bounds;
    }

  // Now we loop thorough all view modules and set the GUISize/WindowPosition.
  foreach(pqGenericViewModule* view, this->Internal->Frames)
    {
    vtkSMIntVectorProperty* prop = 0;

    // set size containing all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("GUISize"));
    if(prop)
      {
      prop->SetElements2(totalBounds.width(), totalBounds.height());
      }

    // position relative to the bounds of all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("WindowPosition"));
    if(prop)
      {
      QPoint view_pos = view->getWidget()->mapToGlobal(QPoint(0,0));
      view_pos -= totalBounds.topLeft();
      prop->SetElements2(view_pos.x(), view_pos.y());
      }
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::saveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* rwRoot = vtkPVXMLElement::New();
  rwRoot->SetName("ViewManager");
  root->AddNestedElement(rwRoot);
  rwRoot->Delete();

  // Save the window layout.
  this->pqMultiView::saveState(rwRoot);

  // Save the render module - window mapping.
  pqInternals::FrameMapType::Iterator iter = this->Internal->Frames.begin();
  for(; iter != this->Internal->Frames.end(); ++iter)
    {
    pqMultiViewFrame* frame = iter.key();
    pqGenericViewModule* view = iter.value();

    pqMultiView::Index index = this->indexOf(frame);
    vtkPVXMLElement* frameElem = vtkPVXMLElement::New();
    frameElem->SetName("Frame");
    frameElem->AddAttribute("index", index.getString().toAscii().data());
    frameElem->AddAttribute("view_module", view->getProxy()->GetSelfIDAsString());
    rwRoot->AddNestedElement(frameElem);
    frameElem->Delete();
    }
}

//-----------------------------------------------------------------------------
bool pqViewManager::loadState(vtkPVXMLElement* rwRoot, 
  vtkSMStateLoader* loader)
{
  if (!rwRoot || !rwRoot->GetName() || strcmp(rwRoot->GetName(), "ViewManager"))
    {
    qDebug() << "Argument must be <ViewManager /> element.";
    return false;
    }

  // When state is loaded by the server manager,
  // the View Manager will have already layed out all the view modules
  // using a default/random scheme. The role of this method
  // is to re-arrange all the views based on the layout in the 
  // state file.
  this->Internal->DontCreateDeleteViewsModules = true; 

  // We remove all "randomly" layed out frames. Note that we are not
  // destroying the view modules, only the frames that got created
  // when the server manager state was getting loaded.
  foreach (pqMultiViewFrame* frame, this->Internal->Frames.keys())
    {
    this->removeWidget(frame);
    }
  this->pqMultiView::loadState(rwRoot);
  this->Internal->DontCreateDeleteViewsModules = false; 
  
  this->Internal->Frames.clear();
  this->Internal->PendingFrames.clear();
  for(unsigned int cc=0; cc < rwRoot->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* elem = rwRoot->GetNestedElement(cc);
    if (strcmp(elem->GetName(), "Frame") == 0)
      {
      QString index_string = elem->GetAttribute("index");

      pqMultiView::Index index;
      index.setFromString(index_string);
      int id = 0;
      elem->GetScalarAttribute("view_module", &id);
      vtkSmartPointer<vtkSMProxy> viewModule;
      viewModule.TakeReference(loader->NewProxy(id));
      if (!viewModule.GetPointer())
        {
        qCritical() << "Failed to locate view module mentioned in state!";
        return false;
        }

      pqGenericViewModule* view = qobject_cast<pqGenericViewModule*>(
        pqApplicationCore::instance()->getServerManagerModel()->getPQProxy(viewModule));
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        this->widgetOfIndex(index));
      if (frame && view)
        {
        this->connect(frame, view);
        frame->setActive(true);
        }
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragStart(pqMultiViewFrame* frame)
{
  QPixmap pixmap(":/pqWidgets/Icons/pqWindow16.png");

  QByteArray output;
  QDataStream dataStream(&output, QIODevice::WriteOnly);
  dataStream << frame->uniqueID();

  QString mimeType = QString("application/paraview3/%1").arg(getpid());

  QMimeData *mimeData = new QMimeData;
  mimeData->setData(mimeType, output);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
  drag->setPixmap(pixmap);

  drag->start();
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragEnter(pqMultiViewFrame*,
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

//-----------------------------------------------------------------------------
void pqViewManager::frameDragMove(pqMultiViewFrame*,
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

//-----------------------------------------------------------------------------
void pqViewManager::frameDrop(pqMultiViewFrame* acceptingFrame,
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
    foreach(f, this->Internal->Frames.keys())
      {
      if(f->uniqueID()==uniqueID)
        {
        originatingFrame=f;
        break;
        }
      }
    if (!originatingFrame)
      {
      foreach (f, this->Internal->PendingFrames)
        {
        if (f->uniqueID() == uniqueID)
          {
          originatingFrame = f;
          break;
          }
        }
      }
    
    if(originatingFrame && originatingFrame != acceptingFrame)
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

      this->updateViewModulePositions();
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

