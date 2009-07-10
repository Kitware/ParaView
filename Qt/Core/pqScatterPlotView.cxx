/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotView.cxx

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
#include "pqScatterPlotView.h"

// Server Manager Includes.
#include "QVTKWidget.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVServerInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMAnimationSceneImageWriter.h"
//#include "vtkSMScatterPlotViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

// Qt Includes.
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QGridLayout>

// ParaView Includes.
#include "pqServer.h"
#include "pqSMAdaptor.h"

pqScatterPlotView::ManipulatorType pqScatterPlotView::TwoDManipulatorTypes[9] = 
{
    { 1, 0, 0, "Pan"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Zoom"},
    { 2, 1, 0, "Zoom"},
    { 3, 1, 0, "Zoom"},
    { 1, 0, 1, "Zoom"},
    { 2, 0, 1, "Zoom"},
    { 3, 0, 1, "Pan"},
};

pqScatterPlotView::ManipulatorType pqScatterPlotView::ThreeDManipulatorTypes[] = 
{
    { 1, 0, 0, "Rotate"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Roll"},
    { 2, 1, 0, "Rotate"},
    { 3, 1, 0, "Pan"},
    { 1, 0, 1, "Zoom"},
    { 2, 0, 1, "Rotate"},
    { 3, 0, 1, "Zoom"},
};

class pqScatterPlotView::pqInternal
{
public:
  QMap<vtkSMViewProxy*, QPointer<QVTKWidget> > RenderWidgets;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  bool ThreeDMode;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    ThreeDMode = false;
    }
};

//-----------------------------------------------------------------------------
pqScatterPlotView::pqScatterPlotView(
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* viewProxy,
  pqServer* server, 
  QObject* _parent):
  Superclass(scatterPlotViewType(), group, name, viewProxy, server, _parent)
{
  this->Internal = new pqInternal();
  //this->Internal->VTKConnect->Connect(
  //  viewProxy, vtkCommand::ConfigureEvent,
  //  this, SLOT(onScatterPlotVisLayoutChanged()));
}

//-----------------------------------------------------------------------------
pqScatterPlotView::~pqScatterPlotView()
{
  foreach (QVTKWidget* widget, this->Internal->RenderWidgets.values())
    {
    delete widget;
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
/*void pqScatterPlotView::initialize()
{
  this->Superclass::initialize();
  //this->onScatterPlotVisLayoutChanged();
}
*/
//-----------------------------------------------------------------------------
void pqScatterPlotView::setDefaultPropertyValues()
{
  //this->getScatterPlotViewProxy()->Build(3, 3);
  this->Superclass::setDefaultPropertyValues();
  /*
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
  */
}

//-----------------------------------------------------------------------------
/*QWidget* pqScatterPlotView::createWidget() 
{
  QWidget* widget = new QWidget();
  return widget;
}
*/
void pqScatterPlotView::initializeWidgets()
{
  this->Superclass::initializeWidgets();
  this->setOrientationAxesVisibility(false);
}

/// Must be overridden to return the default manipulator types.
/*
const pqScatterPlotView::ManipulatorType* pqScatterPlotView
::getDefaultManipulatorTypesInternal()
{
  return pqScatterPlotView::TwoDManipulatorTypes;
}
*/

/*
//-----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy* pqScatterPlotView::getScatterPlotViewProxy() const
{
  return vtkSMScatterPlotViewProxy::SafeDownCast(this->getProxy());
}


//-----------------------------------------------------------------------------
void pqScatterPlotView::onScatterPlotVisLayoutChanged()
{
  // Create QVTKWidgets for new view modules and destroy old ones.
  vtkCollection* currentViews =  vtkCollection::New();
  
  vtkSMScatterPlotViewProxy* compView = vtkSMScatterPlotViewProxy::SafeDownCast(
    this->getProxy());
  compView->GetViews(currentViews);

  QSet<vtkSMViewProxy*> currentViewsSet;

  currentViews->InitTraversal();
  vtkSMViewProxy* temp = vtkSMViewProxy::SafeDownCast(
    currentViews->GetNextItemAsObject());
  for (; temp !=0; temp = vtkSMViewProxy::SafeDownCast(currentViews->GetNextItemAsObject()))
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
    vtkSMRenderViewProxy* renView = vtkSMRenderViewProxy::SafeDownCast(key);
    renView->UpdateVTKObjects();

    QVTKWidget* widget = new QVTKWidget();
    widget->SetRenderWindow(renView->GetRenderWindow());
    widget->installEventFilter(this);
    widget->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->RenderWidgets[key] = widget;
    }

  // Now layout the views.
  int dimensions[2];
  compView->GetDimensions(dimensions);

  // destroy the old layout and create a new one. 
  QWidget* widget = this->getWidget();
  delete widget->layout();

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
      QVTKWidget* vtkwidget = this->Internal->RenderWidgets[view];
      layout->addWidget(vtkwidget, y, x);
      }
    }
  
  currentViews->Delete();

}


//-----------------------------------------------------------------------------
// This method adjusts the extent of a vtkImageData
// using the image's array dimensions and a given top left
// coordinate to start from.
void adjustImageExtent(vtkImageData * image, int topLeftX, int topLeftY)
{
  int extent[6];
  int dimensions[3];
  image->GetDimensions(dimensions);
  extent[0] = topLeftX;
  extent[1] = topLeftX+dimensions[0]-1;
  extent[2] = topLeftY;
  extent[3] = topLeftY+dimensions[1]-1;
  extent[4] = extent[5] = 0;
  image->SetExtent(extent);
}

*/

void pqScatterPlotView::set3DMode(bool enable)
{
  if(enable == this->Internal->ThreeDMode)
    {
    return;
    }
  this->Internal->ThreeDMode = enable;
  this->initializeInteractors();
}

bool pqScatterPlotView::get3DMode()const
{
  return this->Internal->ThreeDMode;
}

const pqScatterPlotView::ManipulatorType* pqScatterPlotView
::getDefaultManipulatorTypesInternal()
{ 
  return this->Internal->ThreeDMode ? 
    pqScatterPlotView::ThreeDManipulatorTypes:
    pqScatterPlotView::TwoDManipulatorTypes; 
}
