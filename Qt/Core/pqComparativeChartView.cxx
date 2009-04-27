/*=========================================================================

   Program: ParaView
   Module:    pqComparativeChartView.cxx

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
#include "pqComparativeChartView.h"

#include "pqServer.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVServerInformation.h"
#include "vtkQtChartWidget.h"
#include "vtkSMChartViewProxy.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QGridLayout>
#include <QWidget>

//-----------------------------------------------------------------------------
pqComparativeChartView::pqComparativeChartView(
  const QString& type, const QString& group, const QString& name, 
  vtkSMComparativeViewProxy* view, pqServer* server, QObject* parentObject)
: Superclass(type, group, name, view, server, parentObject)
{
  this->Widget = new QWidget;
  this->getConnector()->Connect(
    view, vtkCommand::ConfigureEvent,
    this, SLOT(onComparativeVisLayoutChanged()));
}

//-----------------------------------------------------------------------------
pqComparativeChartView::~pqComparativeChartView()
{
  delete this->Widget;
}

//-----------------------------------------------------------------------------
void pqComparativeChartView::initialize()
{
  this->Superclass::initialize();
  this->onComparativeVisLayoutChanged();
}

//-----------------------------------------------------------------------------
/// Returns the internal vtkQtChartView which provide the implementation for
/// the chart rendering.
vtkQtChartView* pqComparativeChartView::getVTKChartView() const
{
  return vtkSMChartViewProxy::SafeDownCast(this->getViewProxy())->GetChartView();
}

//-----------------------------------------------------------------------------
/// Return a widget associated with this view.
QWidget* pqComparativeChartView::getWidget()
{
  return this->Widget;
}

//-----------------------------------------------------------------------------
void pqComparativeChartView::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  vtkPVServerInformation* serverInfo = this->getServer()->getServerInformation();
  if (serverInfo && serverInfo->GetTileDimensions()[0])
    {
    // change default layout to match the tile displays.
    vtkSMPropertyHelper(this->getProxy(),"Dimensions").Set(
      serverInfo->GetTileDimensions(), 2);
    this->getProxy()->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
vtkSMComparativeViewProxy* pqComparativeChartView::getComparativeViewProxy() const
{
  return vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqComparativeChartView::getViewProxy() const
{
  return this->getComparativeViewProxy()->GetRootView();
}

//-----------------------------------------------------------------------------
void pqComparativeChartView::onComparativeVisLayoutChanged()
{
  // Get a collection of current views from the ComparativeViewProxy
  vtkCollection* currentViews =  vtkCollection::New();
  vtkSMComparativeViewProxy* compView = this->getComparativeViewProxy();
  compView->GetViews(currentViews);

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
      vtkSMChartViewProxy* view = vtkSMChartViewProxy::SafeDownCast(
        currentViews->GetItemAsObject(index));
      if (view)
        {
        layout->addWidget(view->GetChartWidget(), y, x);
        }
      }
    }

  // Clean up current views collection
  currentViews->Delete();
}

