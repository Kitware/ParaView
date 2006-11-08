/*=========================================================================

   Program: ParaView
   Module:    pqPlotManager.cxx

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
#include "pqPlotManager.h"

#include <QDockWidget>
#include <QEvent>
#include <QPointer>
#include <QtDebug>
#include <QMainWindow>

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqPlotViewModule.h"

//-----------------------------------------------------------------------------
class pqPlotManagerInternal
{
public:
  QMap<QObject*, pqPlotViewModule*> DockViewMap;
};

//-----------------------------------------------------------------------------
pqPlotManager::pqPlotManager(QObject* _parent/*=0*/)
  : QObject(_parent)
{
  this->Internal = new pqPlotManagerInternal;

  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smModel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onProxyRemoved(pqProxy*)));
}

//-----------------------------------------------------------------------------
pqPlotManager::~pqPlotManager()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPlotManager::onProxyAdded(pqProxy* p)
{
  pqPlotViewModule* plot = qobject_cast<pqPlotViewModule*>(p);
  if (plot)
    {
    this->onPlotAdded(plot);
    }
}

//-----------------------------------------------------------------------------
void pqPlotManager::onProxyRemoved(pqProxy* p)
{
  pqPlotViewModule* plot = qobject_cast<pqPlotViewModule*>(p);
  if (plot)
    {
    this->onPlotRemoved(plot);
    }
}

//-----------------------------------------------------------------------------
void pqPlotManager::onPlotAdded(pqPlotViewModule* p)
{
  QDockWidget* widget = new QDockWidget(p->getSMName(),
    qobject_cast<QWidget*>(this->parent()));
  widget->setObjectName(p->getSMName());
  p->setWindowParent(widget);
  widget->setWidget(p->getWidget());
  widget->show();

  QMainWindow* win = qobject_cast<QMainWindow*>(this->parent());
  if (win)
    {
    win->addDockWidget(Qt::BottomDockWidgetArea, widget);
    }
  else
    {
    widget->setFloating(true);
    }
  p->getWidget()->installEventFilter(this);
  this->Internal->DockViewMap[widget] = p;
  emit this->plotAdded(p);
}

//-----------------------------------------------------------------------------
void pqPlotManager::onPlotRemoved(pqPlotViewModule* p)
{
  emit this->plotRemoved(p);
  this->Internal->DockViewMap.remove(p->getWindowParent());
  delete p->getWindowParent();
}

//-----------------------------------------------------------------------------
bool pqPlotManager::eventFilter(QObject* obj, QEvent* evt)
{
  QWidget* wdg = qobject_cast<QWidget*>(obj);
  if (wdg && evt ->type() == QEvent::FocusIn)
    {
    pqActiveView::instance().setCurrent(this->getViewModule(wdg));
    }

  return QObject::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
pqPlotViewModule* pqPlotManager::getViewModule(QWidget* widget)
{
  foreach (pqPlotViewModule* view, this->Internal->DockViewMap)
    {
    if (view->getWidget() == widget)
      {
      return view;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqPlotManager::renderAllViews()
{
  foreach (pqPlotViewModule* view, this->Internal->DockViewMap)
    {
    if (view)
      {
      view->render();
      }
    }
}
