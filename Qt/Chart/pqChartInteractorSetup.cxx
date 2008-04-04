/*=========================================================================

   Program: ParaView
   Module:    pqChartInteractorSetup.cxx

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

=========================================================================*/

/// \file pqChartInteractorSetup.cxx
/// \date 6/25/2007

#include "pqChartInteractorSetup.h"

#include "pqChartArea.h"
#include "pqChartInteractor.h"
#include "pqChartMouseFunction.h"
#include "pqChartMousePan.h"
#include "pqChartMouseSelection.h"
#include "pqChartMouseZoom.h"


pqChartMouseSelection *pqChartInteractorSetup::createDefault(pqChartArea *area)
{
  // Create a new interactor and add it to the chart area.
  pqChartInteractor *interactor = new pqChartInteractor(area);
  area->setInteractor(interactor);

  // Set up the mouse buttons. Start with pan on the right button.
  interactor->addFunction(new pqChartMousePan(interactor), Qt::RightButton);

  // Add the zoom functionality to the middle button since the middle
  // button usually has the wheel, which is used for zooming.
  interactor->addFunction(new pqChartMouseZoom(interactor), Qt::MidButton);
  interactor->addFunction(new pqChartMouseZoomX(interactor), Qt::MidButton,
      Qt::ControlModifier);
  interactor->addFunction(new pqChartMouseZoomY(interactor), Qt::MidButton,
      Qt::AltModifier);
  interactor->addFunction(new pqChartMouseZoomBox(interactor), Qt::MidButton,
      Qt::ShiftModifier);

  // Add selection to the left button.
  pqChartMouseSelection *selection = new pqChartMouseSelection(interactor);
  interactor->addFunction(selection, Qt::LeftButton);

  return selection;
}

pqChartMouseSelection *pqChartInteractorSetup::createSplitZoom(pqChartArea *area)
{
  // Create a new interactor and add it to the chart area.
  pqChartInteractor *interactor = new pqChartInteractor(area);
  area->setInteractor(interactor);

  // Set up the mouse buttons. Start with pan on the left button.
  interactor->addFunction(new pqChartMousePan(interactor), Qt::LeftButton);

  // Add selection to the left button as well.
  pqChartMouseSelection *selection = new pqChartMouseSelection(interactor);
  interactor->addFunction(selection, Qt::LeftButton);

  // Add the zoom box functionality to the right button.
  interactor->addFunction(new pqChartMouseZoomBox(interactor), Qt::RightButton);

  // Add the rest of the zoom capability to the middle button.
  interactor->addFunction(new pqChartMouseZoom(interactor), Qt::MidButton);
  interactor->addFunction(new pqChartMouseZoomX(interactor), Qt::MidButton,
      Qt::ControlModifier);
  interactor->addFunction(new pqChartMouseZoomY(interactor), Qt::MidButton,
      Qt::AltModifier);

  return selection;
}


