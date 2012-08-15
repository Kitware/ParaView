/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqMultiSliceView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqMultiSliceView.h"

#include <QtCore>
#include <QtGui>

#include "QVTKWidget.h"

#include "pqDataRepresentation.h"
#include "pqMultiSliceAxisWidget.h"
#include "pqRepresentation.h"

#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkDataRepresentation.h"
#include "vtkMath.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRenderView.h"
#include "vtkPlot.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkView.h"

#define MULTI_SLICE_AXIS_THIKNESS 60
#define MULTI_SLICE_AXIS_ACTIVE_SIZE 20
#define MULTI_SLICE_AXIS_EDGE_MARGIN 10

//-----------------------------------------------------------------------------
pqMultiSliceView::pqMultiSliceView(
    const QString& viewType, const QString& group, const QString& name,
    vtkSMViewProxy* viewProxy, pqServer* server, QObject* p)
  : pqRenderView(viewType, group, name, viewProxy, server, p)
{
  // Nothing is changing anything.
  this->UserIsInteracting = false;

  // When data change make sure the bounds are updated
  QObject::connect(this, SIGNAL(updateDataEvent()),
                   this, SLOT(updateAxisBounds()));

  // Make sure all the representations share the same slice values
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
                   this, SLOT(addPropertyListener(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
                   this, SLOT(removePropertyListener(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*,bool)),
                   this, SLOT(updateAxisBounds()));
}

//-----------------------------------------------------------------------------
pqMultiSliceView::~pqMultiSliceView()
{
}
//-----------------------------------------------------------------------------
QWidget* pqMultiSliceView::createWidget()
{
  // Get the internal widget that we want to decorate
  this->InternalWidget = qobject_cast<QVTKWidget*>(this->pqRenderView::createWidget());

  // Build the widget hierarchy
  QWidget* container = new QWidget();
  container->setStyleSheet("background-color: white");
  container->setAutoFillBackground(true);

  QGridLayout* gridLayout = new QGridLayout(container);
  this->InternalWidget->setParent(container);

  // Init top axis
  this->AxisX = new pqMultiSliceAxisWidget(container);
  this->AxisX->setAxisType(vtkAxis::LEFT);
  this->AxisX->setRange(-10,10);
  this->AxisX->setTitle("X");
  this->AxisX->SetEdgeMargin(MULTI_SLICE_AXIS_EDGE_MARGIN);
  this->AxisX->SetActiveSize(MULTI_SLICE_AXIS_ACTIVE_SIZE);
  this->AxisX->setFixedWidth(MULTI_SLICE_AXIS_THIKNESS);
  this->AxisX->renderView();

  this->AxisY = new pqMultiSliceAxisWidget(container);
  this->AxisY->setAxisType(vtkAxis::TOP);
  this->AxisY->setRange(-10,10);
  this->AxisY->setTitle("Y");
  this->AxisY->SetEdgeMargin(MULTI_SLICE_AXIS_EDGE_MARGIN);
  this->AxisY->SetActiveSize(MULTI_SLICE_AXIS_ACTIVE_SIZE);
  this->AxisY->setFixedHeight(MULTI_SLICE_AXIS_THIKNESS - 4);
  this->AxisY->renderView();

  this->AxisZ = new pqMultiSliceAxisWidget(container);
  this->AxisZ->setAxisType(vtkAxis::RIGHT);
  this->AxisZ->setRange(-10,10);
  this->AxisZ->setTitle("Z");
  this->AxisZ->SetEdgeMargin(MULTI_SLICE_AXIS_EDGE_MARGIN);
  this->AxisZ->SetActiveSize(MULTI_SLICE_AXIS_ACTIVE_SIZE);
  this->AxisZ->setFixedWidth(MULTI_SLICE_AXIS_THIKNESS);
  this->AxisZ->renderView();

  gridLayout->addWidget(this->AxisY, 0, 1);  // TOP
  gridLayout->addWidget(this->AxisX, 1, 0); // LEFT
  gridLayout->addWidget(this->AxisZ, 1, 2); // RIGHT
  gridLayout->addWidget(this->InternalWidget,1,1);
  gridLayout->setContentsMargins(0,0,0,0);
  gridLayout->setSpacing(0);

  // Properly do the binding between the proxy and the 3D widget
  vtkSMRenderViewProxy* renModule = this->getRenderViewProxy();
  if (this->InternalWidget && renModule)
    {
    this->InternalWidget->SetRenderWindow(renModule->GetRenderWindow());
    }

  // Make sure we are aware when the slice positions changes
  QObject::connect(this->AxisX, SIGNAL(modelUpdated()),
                   this, SLOT(updateSlices()));
  QObject::connect(this->AxisY, SIGNAL(modelUpdated()),
                   this, SLOT(updateSlices()));
  QObject::connect(this->AxisZ, SIGNAL(modelUpdated()),
                   this, SLOT(updateSlices()));

  return container;
}
//-----------------------------------------------------------------------------
void pqMultiSliceView::updateAxisBounds(double bounds[6])
{
  this->AxisX->setRange(bounds[0], bounds[1]);
  this->AxisY->setRange(bounds[2], bounds[3]);
  this->AxisZ->setRange(bounds[4], bounds[5]);

  // Make sure we render the new range
  this->AxisX->renderView();
  this->AxisY->renderView();
  this->AxisZ->renderView();
}

//-----------------------------------------------------------------------------
void pqMultiSliceView::updateAxisBounds()
{
  double bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                       VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                       VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  double repB[6];
  foreach(pqRepresentation* rep, this->getRepresentations())
    {
    pqDataRepresentation* repData = qobject_cast<pqDataRepresentation*>(rep);
    if(!rep->isVisible() || rep->isWidgetType() || repData == NULL)
      {
      continue;
      }
    vtkPVDataInformation* info = repData->getInputDataInformation();
    if(info)
      {
      info->GetBounds(repB);
      for(int i=0;i<3;i++)
        {
        int index = 2*i;
        bounds[index] = (bounds[index] < repB[index]) ? bounds[index] : repB[index];
        index++;
        bounds[index] = (bounds[index] > repB[index]) ? bounds[index] : repB[index];
        }
      }
    }

  if(bounds[0] != VTK_DOUBLE_MAX)
    {
    this->updateAxisBounds(bounds);
    }
}
//-----------------------------------------------------------------------------
void pqMultiSliceView::updateSlices()
{
  // Make sure we know that the origin of the changes come from the UI
  this->UserIsInteracting = true;

  int nbSliceX = 0;
  const double* sliceX = this->AxisX->getVisibleSlices(nbSliceX);
  int nbSliceY = 0;
  const double* sliceY = this->AxisY->getVisibleSlices(nbSliceY);
  int nbSliceZ = 0;
  const double* sliceZ = this->AxisZ->getVisibleSlices(nbSliceZ);

  foreach(pqRepresentation* rep, this->getRepresentations())
    {
    if( rep->isWidgetType() ||
        !rep->getProxy()->GetProperty("XSlicesValues"))
      {
      continue;
      }
    vtkSMRepresentationProxy* smRep =
        vtkSMRepresentationProxy::SafeDownCast(rep->getProxy());

    vtkSMPropertyHelper(smRep, "XSlicesValues").Set(sliceX, nbSliceX);
    vtkSMPropertyHelper(smRep, "YSlicesValues").Set(sliceY, nbSliceY);
    vtkSMPropertyHelper(smRep, "ZSlicesValues").Set(sliceZ, nbSliceZ);

    smRep->MarkModified(NULL);
    smRep->MarkDirty(NULL);
    smRep->UpdateVTKObjects();
    }
  this->render();

  // Get back to an idle mode
  this->UserIsInteracting = false;
}

//-----------------------------------------------------------------------------
void pqMultiSliceView::addPropertyListener(pqRepresentation* rep)
{
  // Make sure it's a representation we care for
  if( rep->isWidgetType() || !rep->getProxy()->GetProperty("XSlicesValues"))
    {
    return;
    }

  this->ObserverIdX[rep] =
      rep->getProxy()->GetProperty("XSlicesValues")->AddObserver(
        vtkCommand::ModifiedEvent, this, &pqMultiSliceView::updateViewModelCallBack);
  this->ObserverIdY[rep] =
      rep->getProxy()->GetProperty("YSlicesValues")->AddObserver(
        vtkCommand::ModifiedEvent, this, &pqMultiSliceView::updateViewModelCallBack);
  this->ObserverIdZ[rep] =
      rep->getProxy()->GetProperty("ZSlicesValues")->AddObserver(
        vtkCommand::ModifiedEvent, this, &pqMultiSliceView::updateViewModelCallBack);

  // Make sure the added representation get synch with the current view configuration
  int nbSlicesX, nbSlicesY, nbSlicesZ;
  this->AxisX->getVisibleSlices(nbSlicesX);
  this->AxisY->getVisibleSlices(nbSlicesY);
  this->AxisZ->getVisibleSlices(nbSlicesZ);

  // If no slice defined then check if the added representation has some...
  if(nbSlicesX == 0 && nbSlicesY == 0 && nbSlicesZ == 0)
    {
    this->updateViewModelCallBack(NULL, 0, NULL);
    }
  else
    {
    this->updateSlices();
    }
}

//-----------------------------------------------------------------------------
void pqMultiSliceView::removePropertyListener(pqRepresentation* rep)
{
  // Make sure it's a representation we care for
  if( rep->isWidgetType() || !rep->getProxy()->GetProperty("XSlicesValues"))
    {
    return;
    }

  rep->getProxy()->GetProperty("XSlicesValues")->RemoveObserver(this->ObserverIdX[rep]);
  rep->getProxy()->GetProperty("YSlicesValues")->RemoveObserver(this->ObserverIdY[rep]);
  rep->getProxy()->GetProperty("ZSlicesValues")->RemoveObserver(this->ObserverIdZ[rep]);

  this->ObserverIdX.remove(rep);
  this->ObserverIdY.remove(rep);
  this->ObserverIdZ.remove(rep);
}

//-----------------------------------------------------------------------------
void pqMultiSliceView::updateViewModelCallBack(vtkObject*,unsigned long, void*)
{
  if(this->UserIsInteracting)
    {
    return; // We don't want to update our data model
    }

  // Let's pick a valid representation and update the view from it
  bool needToUpdateUI = true;
  foreach(pqRepresentation* rep, this->getRepresentations())
    {
    if(rep->isWidgetType() || !rep->getProxy()->GetProperty("XSlicesValues"))
      {
      continue;
      }
    vtkSMRepresentationProxy* smRep =
        vtkSMRepresentationProxy::SafeDownCast(rep->getProxy());

    if(needToUpdateUI)
      {
      std::vector<double> xSlices =
          vtkSMPropertyHelper(smRep, "XSlicesValues").GetDoubleArray();
      std::vector<double> ySlices =
          vtkSMPropertyHelper(smRep, "YSlicesValues").GetDoubleArray();
      std::vector<double> zSlices =
          vtkSMPropertyHelper(smRep, "ZSlicesValues").GetDoubleArray();

      assert("The maximum number of slice can not be bigger than 255" &&
             ( 255 >
               std::max(
                 std::max(
                   xSlices.size(), ySlices.size()), zSlices.size())));

      // Build a tmp array of visibility set to true
      bool visibility[255];
      memset(visibility, true, 255);

      this->AxisX->updateSlices(&xSlices[0], visibility, static_cast<int>(xSlices.size()));
      this->AxisY->updateSlices(&ySlices[0], visibility, static_cast<int>(ySlices.size()));
      this->AxisZ->updateSlices(&zSlices[0], visibility, static_cast<int>(zSlices.size()));
      }

    smRep->MarkModified(NULL);
    smRep->MarkDirty(NULL);
    smRep->UpdateVTKObjects();
    }

  this->render();
}
