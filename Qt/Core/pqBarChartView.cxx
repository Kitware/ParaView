/*=========================================================================

   Program: ParaView
   Module:    pqBarChartView.cxx

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
#include "pqBarChartView.h"

// Server Manager Includes.
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMBarChartViewProxy.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkPVDataInformation.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"
#include "vtkQtBarChartView.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartTableRepresentation.h"

// ParaView Includes.
#include "pqBarChartRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

// Qt Includes.
#include <QDebug>

//-----------------------------------------------------------------------------
class pqBarChartView::pqInternal
{
public:
  pqInternal()
    {
    }

  ~pqInternal()
    {

    }

  vtkSmartPointer<vtkQtBarChartView>                   BarChartView;
};

//-----------------------------------------------------------------------------
pqBarChartView::pqBarChartView(const QString& group,
                               const QString& name, 
                               vtkSMViewProxy* viewModule,
                               pqServer* server, 
                               QObject* parent/*=NULL*/):
  pqView(barChartViewType(), group, name, viewModule, server, parent)
{
  viewModule->GetID(); // this results in calling CreateVTKObjects().

  this->Internal = new pqInternal();
  this->Internal->BarChartView = vtkSMBarChartViewProxy::SafeDownCast(
    viewModule)->GetBarChartView();

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->Internal->BarChartView->AddChartSelectionHandlers(selector);
  vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());

  // Set up the view undo/redo.
  vtkQtChartContentsSpace *contents =
    this->getVtkBarChartView()->GetChartArea()->getContentsSpace();
  this->connect(contents, SIGNAL(historyPreviousAvailabilityChanged(bool)),
    this, SIGNAL(canUndoChanged(bool)));
  this->connect(contents, SIGNAL(historyNextAvailabilityChanged(bool)),
    this, SIGNAL(canRedoChanged(bool)));
}

//-----------------------------------------------------------------------------
pqBarChartView::~pqBarChartView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqBarChartView::getWidget()
{
  return this->getVtkBarChartView()->GetChartWidget();
}

//-----------------------------------------------------------------------------
vtkQtBarChartView* pqBarChartView::getVtkBarChartView() const
{
  return this->Internal->BarChartView;
}

//-----------------------------------------------------------------------------
void pqBarChartView::setDefaultPropertyValues()
{
  pqView::setDefaultPropertyValues();
}

//-----------------------------------------------------------------------------
void pqBarChartView::undo()
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  area->getContentsSpace()->historyPrevious();
}

//-----------------------------------------------------------------------------
void pqBarChartView::redo()
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  area->getContentsSpace()->historyNext();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canUndo() const
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryPreviousAvailable();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canRedo() const
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryNextAvailable();
}

//-----------------------------------------------------------------------------
void pqBarChartView::resetDisplay()
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  area->getContentsSpace()->resetZoom();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source ? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort || !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
  return (dataInfo && dataInfo->DataSetTypeIsA("vtkDataObject"));
}

