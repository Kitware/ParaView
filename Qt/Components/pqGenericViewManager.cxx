/*=========================================================================

   Program: ParaView
   Module:    pqGenericViewManager.cxx

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
#include "pqGenericViewManager.h"

#include <QDockWidget>
#include <QEvent>
#include <QPointer>
#include <QtDebug>
#include <QMainWindow>

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqGenericViewModule.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqPlotViewModule.h"

//-----------------------------------------------------------------------------
class pqGenericViewManager::pqImplementation
{
public:
  typedef vtkstd::map<pqGenericViewModule*, QDockWidget*> DockViewMapT;
  DockViewMapT DockViewMap;
};

//-----------------------------------------------------------------------------
pqGenericViewManager::pqGenericViewManager(QObject* _parent) :
  QObject(_parent),
  Implementation(new pqImplementation())
{
  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smModel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onProxyRemoved(pqProxy*)));
}

//-----------------------------------------------------------------------------
pqGenericViewManager::~pqGenericViewManager()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqGenericViewManager::onProxyAdded(pqProxy* proxy)
{
  // If this isn't a view, we're done ...
  pqGenericViewModule* const view = qobject_cast<pqGenericViewModule*>(proxy);
  if(!view)
    return;

  // For the short-term, ignore render views ...
  /** \todo Handle render modules here, too */
  if(qobject_cast<pqRenderModule*>(view))
    return;
    
  QDockWidget* const dock_widget = new QDockWidget(
    view->getSMName(),
    qobject_cast<QWidget*>(this->parent()));
  dock_widget->setObjectName(view->getSMName());
  dock_widget->setWidget(view->getWidget());
  dock_widget->show();

//  view->setWindowParent(widget);

  QMainWindow* win = qobject_cast<QMainWindow*>(this->parent());
  if (win)
    {
    win->addDockWidget(Qt::BottomDockWidgetArea, dock_widget);
    }
  else
    {
    dock_widget->setFloating(true);
    }
  view->getWidget()->installEventFilter(this);
  this->Implementation->DockViewMap[view] = dock_widget;
  
  if(pqPlotViewModule* const plot = qobject_cast<pqPlotViewModule*>(view))
    {
    emit this->plotAdded(plot);
    }
}

//-----------------------------------------------------------------------------
void pqGenericViewManager::onProxyRemoved(pqProxy* proxy)
{
  // If this isn't a view, we're done ...
  pqGenericViewModule* const view = qobject_cast<pqGenericViewModule*>(proxy);
  if(!view)
    return;

  // For the short-term, ignore render views ...
  /** \todo Handle render modules here, too */
  if(qobject_cast<pqRenderModule*>(view))
    return;

  if(pqPlotViewModule* const plot = qobject_cast<pqPlotViewModule*>(view))
    {
    emit this->plotRemoved(plot);
    }
  
  view->getWidget()->setParent(0);
  delete this->Implementation->DockViewMap[view];
  this->Implementation->DockViewMap.erase(view);
}

//-----------------------------------------------------------------------------
bool pqGenericViewManager::eventFilter(QObject* obj, QEvent* evt)
{
  QWidget* wdg = qobject_cast<QWidget*>(obj);
  if (wdg && evt ->type() == QEvent::FocusIn)
    {
    for(pqImplementation::DockViewMapT::iterator view =
      this->Implementation->DockViewMap.begin();
      view != this->Implementation->DockViewMap.end();
      ++view)
      {
      if(view->second->widget() == wdg)
        pqActiveView::instance().setCurrent(view->first);
      }

    }

  return QObject::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
void pqGenericViewManager::renderAllViews()
{
  for(pqImplementation::DockViewMapT::iterator view =
    this->Implementation->DockViewMap.begin();
    view != this->Implementation->DockViewMap.end();
    ++view)
    {
    view->first->render();
    }
}
