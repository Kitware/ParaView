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
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVServerInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMComparativeViewProxy.h"
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

class pqComparativeRenderView::pqInternal
{
public:
  QMap<vtkSMViewProxy*, QPointer<pqQVTKWidget> > RenderWidgets;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqComparativeRenderView::pqComparativeRenderView(
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* viewProxy,
  pqServer* server, 
  QObject* _parent):
  Superclass(comparativeRenderViewType(), group, name, viewProxy, server, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::ConfigureEvent,
    this, SLOT(onComparativeVisLayoutChanged()));
}

//-----------------------------------------------------------------------------
pqComparativeRenderView::~pqComparativeRenderView()
{
  foreach (pqQVTKWidget* widget, this->Internal->RenderWidgets.values())
    {
    delete widget;
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::initialize()
{
  this->Superclass::initialize();
  this->onComparativeVisLayoutChanged();
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::setDefaultPropertyValues()
{
  //this->getComparativeRenderViewProxy()->Build(3, 3);
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
QWidget* pqComparativeRenderView::createWidget() 
{
  QWidget* widget = new QWidget();
  return widget;
}

//-----------------------------------------------------------------------------
vtkSMComparativeViewProxy* pqComparativeRenderView::getComparativeRenderViewProxy() const
{
  return vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqComparativeRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(
    this->getComparativeRenderViewProxy()->GetRootView());
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::onComparativeVisLayoutChanged()
{
  // Create QVTKWidgets for new view modules and destroy old ones.
  vtkCollection* currentViews =  vtkCollection::New();
  
  vtkSMComparativeViewProxy* compView = vtkSMComparativeViewProxy::SafeDownCast(
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
    pqQVTKWidget* item = this->Internal->RenderWidgets.take(key);
    delete item;
    }

  // Create QVTKWidgets for new ones.
  foreach (vtkSMViewProxy* key, added)
    {
    vtkSMRenderViewProxy* renView = vtkSMRenderViewProxy::SafeDownCast(key);
    renView->UpdateVTKObjects();

    pqQVTKWidget* widget = new pqQVTKWidget();
    widget->SetRenderWindow(renView->GetRenderWindow());
    widget->setSession(compView->GetSession());
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
  for (int x=0; x < dimensions[0]; x++)
    {
    for (int y=0; y < dimensions[1]; y++)
      {
      int index = y*dimensions[0]+x;
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
        currentViews->GetItemAsObject(index));
      pqQVTKWidget* vtkwidget = this->Internal->RenderWidgets[view];
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


//-----------------------------------------------------------------------------
vtkImageData* pqComparativeRenderView::captureImage(int magnification)
{
  if (!this->getWidget()->isVisible())
    {
    // Don't return any image when the view is not visible.
    return NULL;
    }

  // This method will capture the rendered image from each
  // internal view and combine them together to create the final image.
  QList<vtkImageData*> images;

  // Get the collection of view proxies
  int dimensions[2];
  vtkSMComparativeViewProxy* compView = this->getComparativeRenderViewProxy();

  vtkCollection* currentViews =  vtkCollection::New();
  compView->GetViews(currentViews);
  vtkSMPropertyHelper( compView, "Dimensions").Get(dimensions, 2);
  if (vtkSMPropertyHelper(compView, "OverlayAllComparisons").GetAsInt() !=0)
    {
    dimensions[0] = dimensions[1] = 1;
    }


  int imageSize[3];
  int finalImageSize[2] = {0, 0};

  // For each view proxy...
  for (int y=0; y < dimensions[1]; y++)
    {
    // reset x component to zero when starting a new row
    finalImageSize[0] = 0;
    for (int x=0; x < dimensions[0]; x++)
      {
      int index = y*dimensions[0]+x;
      vtkSMRenderViewProxy* view = vtkSMRenderViewProxy::SafeDownCast(
        currentViews->GetItemAsObject(index));
      if (view)
        {
        // There seems to be a bug where offscreen rendering
        // does not work with comparative view screenshots, so we
        // will force offscreen rendering off... FIXME!
        vtkSMPropertyHelper(view, "UseOffscreenRenderingForScreenshots").Set(0);

        // Capture the image
        vtkImageData * image = view->CaptureWindow(magnification);

        // There is a bug where the cloned views do not correctly
        // track their own view position (and therefore extent),
        // so for now we'll manually adjust their extent information.
        adjustImageExtent(image, finalImageSize[0], finalImageSize[1]);
        image->GetDimensions(imageSize);
        finalImageSize[0] += imageSize[0];

        // Append the image to the list
        images.append(image);
        }
      }
      finalImageSize[1] += imageSize[1]; 
    }

  // Allocate final image data
  vtkImageData * finalImage = vtkImageData::New();
  finalImage->SetDimensions(finalImageSize[0], finalImageSize[1], 1);
  finalImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  // Append all images together
  foreach (vtkImageData * image, images)
    {
    vtkSMAnimationSceneImageWriter::Merge(finalImage, image);
    image->Delete();
    }

  // Update final image extent based on view position and magnification
  int viewPosition[2];
  int extentFinal[6];
  vtkSMPropertyHelper(this->getRenderViewProxy(), "ViewPosition").Get(viewPosition, 2);
  finalImage->GetExtent(extentFinal);
  for (int cc=0; cc < 4; cc++)
    {
    extentFinal[cc] += viewPosition[cc/2]*magnification;
    }
  finalImage->SetExtent(extentFinal);

  // Clean up views collection
  currentViews->Delete();

  return finalImage;
}

