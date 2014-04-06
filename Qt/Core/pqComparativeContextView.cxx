/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqComparativeContextView.h"

#include "pqServer.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVServerInformation.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "QVTKWidget.h"
#include "vtkContextView.h"
#include "vtkSmartPointer.h"

#include <QMap>
#include <QPointer>
#include <QSet>
#include <QGridLayout>
#include <QWidget>

class pqComparativeContextView::pqInternal
{
public:
  QMap<vtkSMViewProxy*, QPointer<QVTKWidget> > RenderWidgets;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqComparativeContextView::pqComparativeContextView(const QString& type,
                                                   const QString& group,
                                                   const QString& name,
                                                   vtkSMComparativeViewProxy* view,
                                                   pqServer* server,
                                                   QObject* parentObject)
  : Superclass(type, group, name, view, server, parentObject)
{
  this->Internal = new pqInternal();
  this->Widget = new QWidget;
  this->getConnector()->Connect(view, vtkCommand::ConfigureEvent,
                                this, SLOT(onComparativeVisLayoutChanged()));
}

//-----------------------------------------------------------------------------
pqComparativeContextView::~pqComparativeContextView()
{
  foreach (QVTKWidget* widget, this->Internal->RenderWidgets.values())
    {
    delete widget;
    }
  delete this->Internal;
  delete this->Widget;
}

//-----------------------------------------------------------------------------
void pqComparativeContextView::initialize()
{
  this->Superclass::initialize();
  this->onComparativeVisLayoutChanged();
}

//-----------------------------------------------------------------------------
vtkContextView* pqComparativeContextView::getVTKContextView() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getViewProxy())
      ->GetContextView();
}

//-----------------------------------------------------------------------------
vtkSMContextViewProxy* pqComparativeContextView::getContextViewProxy() const
{
 return vtkSMContextViewProxy::SafeDownCast(this->getViewProxy());
}

//-----------------------------------------------------------------------------
QWidget* pqComparativeContextView::getWidget()
{
  return this->Widget;
}

//-----------------------------------------------------------------------------
vtkSMComparativeViewProxy* pqComparativeContextView::getComparativeViewProxy() const
{
  return vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqComparativeContextView::getViewProxy() const
{
  return this->getComparativeViewProxy()->GetRootView();
}

//-----------------------------------------------------------------------------
void pqComparativeContextView::onComparativeVisLayoutChanged()
  {
  // This logic is adapted from pqComparativeRenderView, the two should be
  // consolidated/refactored to have a common base class.
  // Create QVTKWidgets for new view modules and destroy old ones.
  vtkCollection* currentViews =  vtkCollection::New();

  vtkSMComparativeViewProxy* compView = vtkSMComparativeViewProxy::SafeDownCast(
    this->getProxy());
  compView->GetViews(currentViews);

  QSet<vtkSMViewProxy*> currentViewsSet;

  currentViews->InitTraversal();
  vtkSMViewProxy* temp = vtkSMViewProxy::SafeDownCast(
    currentViews->GetNextItemAsObject());
  for (; temp !=0; temp =
       vtkSMViewProxy::SafeDownCast(currentViews->GetNextItemAsObject()))
    {
    currentViewsSet.insert(temp);
    }

  QSet<vtkSMViewProxy*> oldViews = QSet<vtkSMViewProxy*>::fromList(
    this->Internal->RenderWidgets.keys());

  QSet<vtkSMViewProxy*> removed = oldViews - currentViewsSet;
  QSet<vtkSMViewProxy*> added = currentViewsSet - oldViews;

  // Destroy old QVTKWidgets widgets.
  foreach (vtkSMViewProxy* key, removed)
    {
    QVTKWidget* item = this->Internal->RenderWidgets.take(key);
    delete item;
    }

  // Create QVTKWidgets for new ones.
  foreach (vtkSMViewProxy* key, added)
    {
    vtkSMContextViewProxy* renView = vtkSMContextViewProxy::SafeDownCast(key);
    renView->UpdateVTKObjects();

    QVTKWidget* widget = new QVTKWidget();
    renView->GetContextView()->SetInteractor(widget->GetInteractor());
    widget->SetRenderWindow(renView->GetContextView()->GetRenderWindow());
    widget->installEventFilter(this);
    widget->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->RenderWidgets[key] = widget;
    }

  // Now layout the views.
  int dimensions[2];
  vtkSMPropertyHelper(compView, "Dimensions").Get(dimensions, 2);
  if (vtkSMPropertyHelper(compView, "OverlayAllComparisons").GetAsInt() !=0)
    {
    dimensions[0] = dimensions[1] = 1;
    }

  // destroy the old layout and create a new one.
  QWidget* widget = this->getWidget();
  delete widget->layout();

  QGridLayout* layout = new QGridLayout(widget);
  layout->setSpacing(1);
  layout->setMargin(0);
  for (int x = 0; x < dimensions[0]; ++x)
    {
    for (int y = 0; y < dimensions[1]; ++y)
      {
      int index = y*dimensions[0]+x;
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
        currentViews->GetItemAsObject(index));
      QVTKWidget* vtkwidget = this->Internal->RenderWidgets[view];
      layout->addWidget(vtkwidget, y, x);
      }
    }

  currentViews->Delete();
}
