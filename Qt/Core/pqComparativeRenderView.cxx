/*=========================================================================

   Program: ParaView
   Module:    pqComparativeRenderView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqComparativeRenderView.h"

// Server Manager Includes.
#include "pqQVTKWidget.h"
#include "pqUndoStack.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVServerInformation.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

// Qt Includes.
#include <QGridLayout>
#include <QMap>
#include <QPointer>
#include <QSet>

// ParaView Includes.
#include "pqSMAdaptor.h"
#include "pqServer.h"

class pqComparativeRenderView::pqInternal
{
public:
  QMap<vtkSMViewProxy*, QPointer<pqQVTKWidget> > RenderWidgets;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal() { this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New(); }
};

namespace
{
/// This helps us monitor QResizeEvent after it has been processed (unlike a
/// generic event filter).
class pqComparativeWidget : public QWidget
{
public:
  vtkWeakPointer<vtkSMProxy> ViewProxy;
  void resizeEvent(QResizeEvent* evt) override
  {
    this->QWidget::resizeEvent(evt);

    QSize asize = this->size() * this->devicePixelRatio();
    int view_size[2];
    view_size[0] = asize.width();
    view_size[1] = asize.height();

    BEGIN_UNDO_EXCLUDE();
    vtkSMPropertyHelper(this->ViewProxy, "ViewSize").Set(view_size, 2);
    this->ViewProxy->UpdateProperty("ViewSize");
    END_UNDO_EXCLUDE();
  }
};
}
//-----------------------------------------------------------------------------
pqComparativeRenderView::pqComparativeRenderView(const QString& group, const QString& name,
  vtkSMViewProxy* viewProxy, pqServer* server, QObject* _parent)
  : Superclass(comparativeRenderViewType(), group, name, viewProxy, server, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::ConfigureEvent, this, SLOT(updateViewWidgets()));
}

//-----------------------------------------------------------------------------
pqComparativeRenderView::~pqComparativeRenderView()
{
  foreach (pqQVTKWidget* wdg, this->Internal->RenderWidgets.values())
  {
    delete wdg;
  }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqComparativeRenderView::createWidget()
{
  pqComparativeWidget* container = new pqComparativeWidget();
  container->ViewProxy = this->getProxy();
  this->updateViewWidgets(container);
  return container;
}

//-----------------------------------------------------------------------------
vtkSMComparativeViewProxy* pqComparativeRenderView::getComparativeRenderViewProxy() const
{
  return vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqComparativeRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(this->getComparativeRenderViewProxy()->GetRootView());
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::updateViewWidgets(QWidget* container /*=NULL*/)
{
  // Create QVTKWidgets for new view modules and destroy old ones.
  vtkNew<vtkCollection> currentViews;
  vtkSMComparativeViewProxy* compView = vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
  compView->GetViews(currentViews.GetPointer());

  QSet<vtkSMViewProxy*> currentViewsSet;

  currentViews->InitTraversal();
  vtkSMViewProxy* temp = vtkSMViewProxy::SafeDownCast(currentViews->GetNextItemAsObject());
  for (; temp != 0; temp = vtkSMViewProxy::SafeDownCast(currentViews->GetNextItemAsObject()))
  {
    currentViewsSet.insert(temp);
  }

  QSet<vtkSMViewProxy*> oldViews =
    QSet<vtkSMViewProxy*>::fromList(this->Internal->RenderWidgets.keys());

  QSet<vtkSMViewProxy*> removed = oldViews - currentViewsSet;
  QSet<vtkSMViewProxy*> added = currentViewsSet - oldViews;

  // Destroy old QVTKWidgets widgets.
  foreach (vtkSMViewProxy* key, removed)
  {
    pqQVTKWidget* item = this->Internal->RenderWidgets.take(key);
    delete item;
  }

  // Create QVTKWidgets for new ones.
  foreach (vtkSMViewProxy* key, added)
  {
    vtkSMRenderViewProxy* renView = vtkSMRenderViewProxy::SafeDownCast(key);
    renView->UpdateVTKObjects();

    pqQVTKWidget* wdg = new pqQVTKWidget();
    wdg->setRenderWindow(renView->GetRenderWindow());
    renView->SetupInteractor(wdg->interactor());
    wdg->setSession(compView->GetSession());
    wdg->installEventFilter(this);
    wdg->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->RenderWidgets[key] = wdg;
  }

  // Now layout the views.
  int dimensions[2];
  vtkSMPropertyHelper(compView, "Dimensions").Get(dimensions, 2);
  if (vtkSMPropertyHelper(compView, "OverlayAllComparisons").GetAsInt() != 0)
  {
    dimensions[0] = dimensions[1] = 1;
  }

  // destroy the old layout and create a new one.
  container = container ? container : this->widget();
  delete container->layout();

  QGridLayout* layout = new QGridLayout(container);
  layout->setHorizontalSpacing(vtkSMPropertyHelper(compView, "Spacing").GetAsInt(0));
  layout->setVerticalSpacing(vtkSMPropertyHelper(compView, "Spacing").GetAsInt(1));
  layout->setMargin(0);
  for (int x = 0; x < dimensions[0]; x++)
  {
    for (int y = 0; y < dimensions[1]; y++)
    {
      int index = y * dimensions[0] + x;
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(currentViews->GetItemAsObject(index));
      pqQVTKWidget* vtkwidget = this->Internal->RenderWidgets[view];
      layout->addWidget(vtkwidget, y, x);
    }
  }
}
