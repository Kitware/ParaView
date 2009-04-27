/*=========================================================================

   Program: ParaView
   Module:    pqComparativePlotView.cxx

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
#include "pqComparativePlotView.h"

// Server Manager Includes.
#include "QVTKWidget.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVServerInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMRepresentationProxy.h"

// Qt Includes.
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QGridLayout>

// ParaView Includes.
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqRepresentation.h"
//#include "pqLineChartRepresentation.h"
//#include "pqBarChartRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"
#include "pqChartWidget.h"
#include "pqChartArea.h"

class pqComparativePlotView::pqInternal
{
public:

  // Since we use internal proxies that are not registered
  // with the server manager, we must maintain our own
  // mappings of SMProxy to pqObject
  //QMap<vtkSMViewProxy*, QPointer<pqPlotView> > ViewMap;
  QMap<vtkSMRepresentationProxy*, pqRepresentation*> RepresentationMap;

  // This map allows us to determine which reprs are to be added/removed
  // during a call to onRepresentationsChanged.  It also lets us remove
  // all representations from a view when the view is removed.
  QMap<pqView*, QList<vtkSMRepresentationProxy*> > ViewRepresentationMap;

  // Since pqRepresentation::setView is protected, we cannot call it on
  // the internally created pqRepresentations, so we must keep track
  // of what view the pqRepr belongs.
  //QMap<pqRepresentation*, pqPlotView* > RepresentationViewMap;

  // This list is for convenience.  It makes it easier to look up
  // which pqReprs were created internally.
  QList< QPointer<pqRepresentation> > CreatedRepresentations;

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqComparativePlotView::pqComparativePlotView(
  const QString& type,
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* viewProxy,
  pqServer* server, 
  QObject* _parent):
  Superclass(type, group, name, viewProxy, server, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::ConfigureEvent,
    this, SLOT(onComparativeVisLayoutChanged()));
  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::UserEvent,
    this, SLOT(representationsChanged()));

}


//-----------------------------------------------------------------------------
pqComparativePlotView::~pqComparativePlotView()
{
  /*
  foreach (pqPlotView* view, this->Internal->ViewMap.values())
    {
    delete view;
    }

  foreach (pqRepresentation* repr, this->Internal->CreatedRepresentations)
    {
    delete repr;
    }

  delete this->Internal;
  */
}

//-----------------------------------------------------------------------------
void pqComparativePlotView::initialize()
{
  this->Superclass::initialize();

  this->connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation *, bool)),
    this, SLOT(updateVisibility()));

  this->onComparativeVisLayoutChanged();
}

//-----------------------------------------------------------------------------
void pqComparativePlotView::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  vtkPVServerInformation* serverInfo = this->getServer()->getServerInformation();
  if (serverInfo && serverInfo->GetTileDimensions()[0])
    {
    // change default layout to match the tile displays.
    pqSMAdaptor::setMultipleElementProperty(
      this->getProxy()->GetProperty("Dimensions"), 0, 
      serverInfo->GetTileDimensions()[0]);
    pqSMAdaptor::setMultipleElementProperty(
      this->getProxy()->GetProperty("Dimensions"), 1, 
      serverInfo->GetTileDimensions()[1]);
    this->getProxy()->UpdateVTKObjects();
    }

}

//-----------------------------------------------------------------------------
vtkSMComparativeViewProxy* pqComparativePlotView::getComparativeViewProxy() const
{
  return vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqComparativePlotView::getViewProxy() const
{
  return this->getComparativeViewProxy()->GetRootView();
}

//-----------------------------------------------------------------------------
void pqComparativePlotView::updateVisibility()
{
  this->getComparativeViewProxy()->UpdateVisualization(1);
}

//-----------------------------------------------------------------------------
void pqComparativePlotView::onComparativeVisLayoutChanged()
{
  /*
  // Get a collection of current views from the ComparativeViewProxy
  vtkCollection* currentViews =  vtkCollection::New();
  vtkSMComparativeViewProxy* compView = this->getComparativeViewProxy();
  compView->GetViews(currentViews);

  // Declare the set of current views
  QSet<vtkSMViewProxy*> currentViewsSet;

  // Insert each view from current views collection into the current views Sst
  currentViews->InitTraversal();
  vtkSMViewProxy* temp = vtkSMViewProxy::SafeDownCast(
    currentViews->GetNextItemAsObject());
  for (; temp !=0; temp = vtkSMViewProxy::SafeDownCast(currentViews->GetNextItemAsObject()))
    {
    currentViewsSet.insert(temp);
    }

  // Declare the set of old views
  // Old views are the keys of the ViewMap
  QSet<vtkSMViewProxy*> oldViews = QSet<vtkSMViewProxy*>::fromList(
    this->Internal->ViewMap.keys());
  
  // Declare the sets of views that are to be added and to be removed.
  QSet<vtkSMViewProxy*> removed = oldViews - currentViewsSet;
  QSet<vtkSMViewProxy*> added = currentViewsSet - oldViews;

  // For each view to be removed, destroy its pqPlotView.
  foreach (vtkSMViewProxy* key, removed)
    {
    pqPlotView * plotView = this->Internal->ViewMap.take(key);
    QList<vtkSMRepresentationProxy*> reprProxies =
      this->Internal->ViewRepresentationMap.take(plotView);

    // Destroy all reprs in the view
    QList<vtkSMRepresentationProxy*>::const_iterator reprIterator;
    for (reprIterator = reprProxies.begin(); reprIterator != reprProxies.end(); ++reprIterator)
      {
      pqRepresentation * pqRepr = this->Internal->RepresentationMap.take(*reprIterator);
      // Don't need to remove the repr from the view because we are deleting the view.
      delete pqRepr;
      }
    delete plotView;
    }

  // For each view to be added, create its pqPlotView
  foreach (vtkSMViewProxy* view, added)
    {
    view->UpdateVTKObjects();
    pqPlotView * plotView = new pqPlotView(this->getViewType(),
                                  this->getSMGroup(),
                                  "name",
                                  view,
                                  this->getServer());

    // Initialize just calls onRepresentationsChanged, but
    // we handle that ourselves internally, so we don't really
    // need to call initialize.  Calling initialize would requre
    // that pqPlotView lists us as a friend class.
    //plotView->initialize();

    // When the charts set their title text, we might want to insert
    // some values into the text, such as the value of the comparative variable
    this->connect(plotView, SIGNAL(beginSetTitleText(const pqPlotView*, QString&)),
      this, SLOT(adjustTitleText(const pqPlotView*, QString&)) );

    // Store the plotview in the map
    this->Internal->ViewMap[view] = plotView;

    // Connect context menu requests
    pqChartWidget *chart = qobject_cast<pqChartWidget *>(plotView->getWidget());
    pqChartArea *area = chart->getChartArea();
    area->setContextMenuPolicy(Qt::CustomContextMenu);
    chart = qobject_cast<pqChartWidget *>(this->getWidget());
    this->connect(area, SIGNAL(customContextMenuRequested(const QPoint &)),
      chart->getChartArea(), SIGNAL(customContextMenuRequested(const QPoint &)));

    }

  // Get dimensions for new view layout
  int dimensions[2];
  compView->GetDimensions(dimensions);

  // Destroy the old layout.
  QWidget* widget = this->getWidget();
  delete widget->layout();

  // Create the new layout
  QGridLayout* layout = new QGridLayout(widget);
  layout->setSpacing(1);
  layout->setMargin(0);
  for (int x=0; x < dimensions[0]; x++)
    {
    for (int y=0; y < dimensions[1]; y++)
      {
      int index = y*dimensions[0]+x;
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
        currentViews->GetItemAsObject(index));
      pqPlotView* plotView = this->Internal->ViewMap[view];
      layout->addWidget(plotView->getWidget(), y, x);
      }
    }

  // Clean up current views collection
  currentViews->Delete();

  this->representationsChanged();
  */
}

//-----------------------------------------------------------------------------
void pqComparativePlotView::representationsChanged()
{
  /*
  // Get the server manager model
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  // Add or remove representations to each view
  // For each view...
  foreach (vtkSMViewProxy* view, this->Internal->ViewMap.uniqueKeys())
    {

    // Get the pqView for this view proxy
    pqPlotView * plotView = this->Internal->ViewMap[view];

    // Get all the representation proxies currently in this view
    vtkCollection* proxyCollection =  vtkCollection::New();
    this->getComparativeViewProxy()->GetRepresentationsForView(view, proxyCollection);

    // Convert the vtkCollection to a QSet
    QSet<vtkSMRepresentationProxy*> currentReprProxies;
    proxyCollection->InitTraversal();
    vtkSMRepresentationProxy* temp =
      vtkSMRepresentationProxy::SafeDownCast(proxyCollection->GetNextItemAsObject());
    for (; temp != 0;
      temp = vtkSMRepresentationProxy::SafeDownCast(proxyCollection->GetNextItemAsObject()))
      {
        currentReprProxies.insert(temp);
      }

    // Clean up the collection
    proxyCollection->Delete();

    // Get the set of old set of representation proxies.
    // Note, some of these could be dangling pointers
    // but we're only using them as keys to a map.
    QSet<vtkSMRepresentationProxy*> oldReprProxies =
        this->Internal->ViewRepresentationMap[plotView].toSet();

    // Define the sets of representations to be added and removed.
    QSet<vtkSMRepresentationProxy*> added = currentReprProxies - oldReprProxies;
    QSet<vtkSMRepresentationProxy*> removed = oldReprProxies - currentReprProxies;

    // For each representation to be added...
    QSet<vtkSMRepresentationProxy*>::const_iterator reprIterator;
    for (reprIterator = added.begin(); reprIterator != added.end(); ++reprIterator)
      {
      vtkSMRepresentationProxy * reprProxy = *reprIterator;
      pqRepresentation * pqRepr = 0;

      // If its the root view, then we'll look up the pqRepresentation
      // from the server manager model
      if (view == this->getViewProxy())
        {
        pqRepr =  smModel->findItem<pqRepresentation*>(reprProxy);
        }
      // Else the view is not the root view, so we'll create our own pqRepresentation
      else if (this->getViewType() == "XYPlotView")
        {
        pqRepr = new pqLineChartRepresentation(this->getSMGroup(), "name", reprProxy, this->getServer());
        this->Internal->CreatedRepresentations.append(pqRepr);
        }
      else if (this->getViewType() == "BarChartView")
        {
        pqRepr = new pqBarChartRepresentation(this->getSMGroup(), "name", reprProxy, this->getServer());
        this->Internal->CreatedRepresentations.append(pqRepr);
        }

      if (pqRepr)
        {
        // Add the pqRepresentation and keep track of it in our maps
        plotView->addRepresentation(pqRepr);
        this->Internal->ViewRepresentationMap[plotView].append(reprProxy);
        this->Internal->RepresentationViewMap[pqRepr] = plotView;
        this->Internal->RepresentationMap[reprProxy] = pqRepr;
        }
      }

    // Delete old representations
    for (reprIterator = removed.begin(); reprIterator != removed.end(); ++reprIterator)
      {
      vtkSMRepresentationProxy * reprProxy = *reprIterator;

      // Look up the representation and remove it from the map
      pqRepresentation * pqRepr = this->Internal->RepresentationMap.take(reprProxy);

      // Remove the representation from its pqView
      pqPlotView * tempPlotView = this->Internal->RepresentationViewMap[pqRepr];
      tempPlotView->removeRepresentation(pqRepr);

      // Delete the pqRepresentation if it is one we created.
      if (this->Internal->CreatedRepresentations.contains(pqRepr))
        {
        delete pqRepr;
        }

      }

    }
  */
}
/*
//-----------------------------------------------------------------------------
void pqComparativePlotView::adjustTitleText(const pqPlotView * plotView, QString & titleText)
{
  vtkSMProperty * prop = 0;
  int elementNum = 0;
  if (titleText.contains("%xprop%", Qt::CaseInsensitive)
      && this->getComparativeViewProxy()->GetXPropertyAndElement(prop, elementNum))
    {
    QVariant value = pqSMAdaptor::getMultipleElementProperty(prop, elementNum);
    QString replacement = QString("%1").arg(value.toString());
    titleText.replace("%xprop%", replacement);
    }

  if (titleText.contains("%yprop%", Qt::CaseInsensitive)
      && this->getComparativeViewProxy()->GetYPropertyAndElement(prop, elementNum))
    {
    QVariant value = pqSMAdaptor::getMultipleElementProperty(prop, elementNum);
    QString replacement = QString("%1").arg(value.toString());
    titleText.replace("%yprop%", replacement);
    }

  if (titleText.contains("%time%", Qt::CaseInsensitive))
    {
    vtkSMViewProxy * viewProxy = vtkSMViewProxy::SafeDownCast(plotView->getProxy());
    QString replacement = QString("%1").arg(viewProxy->GetViewUpdateTime());
    titleText.replace("%time%", replacement);
    }
}
*/


