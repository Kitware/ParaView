/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientLineChartView.h"

#include <vtkSMClientDeliveryRepresentationProxy.h>
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include <vtkSMViewProxy.h>

#include <pqRepresentation.h>
#include "pqSMAdaptor.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtLineChartView.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartInteractor.h"
#include "vtkQtChartMouseZoom.h"
#include "vtkQtChartMousePan.h"
#include "vtkQtChartMouseSelection.h"


////////////////////////////////////////////////////////////////////////////////////
// ClientLineChartView

ClientLineChartView::ClientLineChartView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  ClientChartView(viewmoduletype, group, name, viewmodule, server, p)
{
  this->ChartView = vtkQtLineChartView::New();

  // Set the chart color scheme to custom so we can use our own lookup table
  //vtkQtChartStyleManager *style = this->ChartView->GetChartArea()->getStyleManager();
  //vtkQtChartColorStyleGenerator *gen = new vtkQtChartColorStyleGenerator(style, vtkQtChartColors::Custom);
  //style->setGenerator(gen);

  // Set up the default interactor.
  // Create a new interactor and add it to the chart area.
  vtkQtChartInteractor *interactor = new vtkQtChartInteractor(this->ChartView->GetChartArea());
  this->ChartView->GetChartArea()->setInteractor(interactor);

  // Set up the mouse buttons. Start with pan on the right button.
  interactor->addFunction(Qt::RightButton, new vtkQtChartMouseZoom(interactor));
  interactor->addFunction(Qt::RightButton, new vtkQtChartMouseZoomX(interactor),
      Qt::ControlModifier);
  interactor->addFunction(Qt::RightButton, new vtkQtChartMouseZoomY(interactor),
      Qt::AltModifier);

  // Add the zoom functionality to the middle button since the middle
  // button usually has the wheel, which is used for zooming.
  interactor->addFunction(Qt::MidButton, new vtkQtChartMousePan(interactor));

  // Add zoom functionality to the wheel.
  interactor->addWheelFunction(new vtkQtChartMouseZoom(interactor));
  interactor->addWheelFunction(new vtkQtChartMouseZoomX(interactor),
      Qt::ControlModifier);
  interactor->addWheelFunction(new vtkQtChartMouseZoomY(interactor),
      Qt::AltModifier);

  this->ChartView->AddChartSelectionHandlers(new vtkQtChartMouseSelection(interactor));

  QObject::connect(this->ChartView->GetChartArea()->getContentsSpace(),
                   SIGNAL(historyPreviousAvailabilityChanged(bool)),
                   this,
                   SIGNAL(canUndoChanged(bool)));
  QObject::connect(this->ChartView->GetChartArea()->getContentsSpace(),
                   SIGNAL(historyNextAvailabilityChanged(bool)),
                   this,
                   SIGNAL(canRedoChanged(bool)));
}

ClientLineChartView::~ClientLineChartView()
{
}
