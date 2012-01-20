/*=========================================================================

   Program: ParaView
   Module:    pqChartSelectionReaction.cxx

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
#include "pqChartSelectionReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqContextView.h"
#include "vtkChart.h"
#include "vtkContextScene.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkSMContextViewProxy.h"

//-----------------------------------------------------------------------------
pqChartSelectionReaction::pqChartSelectionReaction(
  QAction* parentObject, pqContextView* view, int selectionMode)
  : Superclass(parentObject), View(view), SelectionMode(selectionMode)
{
  if (!view)
    {
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
      this, SLOT(updateEnableState()), Qt::QueuedConnection);
    }

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::updateEnableState()
{
  pqView *view = this->View;
  if (!view)
    {
    view = pqActiveObjects::instance().activeView();
    }
  pqContextView *thisView = qobject_cast<pqContextView*>(view);
  if (thisView && thisView->supportsSelection())
    {
    this->parentAction()->setEnabled(true);
    }
  else
    {
    this->parentAction()->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::startSelection(
  pqContextView* view, int selMode)
{
  if(view && view->supportsSelection() && view->getContextViewProxy())
    {
    vtkAbstractContextItem* contextItem =
      view->getContextViewProxy()->GetContextItem();
    vtkChart *chart = vtkChart::SafeDownCast(contextItem);
    vtkScatterPlotMatrix *chartMatrix =
      vtkScatterPlotMatrix::SafeDownCast(contextItem);
    if (chart)
      {
      chart->SetSelectionMode(selMode);
      }
    else if(chartMatrix)
      {
      chartMatrix->SetSelectionMode(selMode);
      }
    }
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::onTriggered()
{ 
  if(this->View->supportsSelection() && this->View->getContextViewProxy())
    {
    vtkAbstractContextItem* contextItem =
      this->View->getContextViewProxy()->GetContextItem();
    vtkChart *chart = vtkChart::SafeDownCast(contextItem);
    vtkScatterPlotMatrix *chartMatrix =
      vtkScatterPlotMatrix::SafeDownCast(contextItem);
    int selMode = -1;
    if (chart)
      {
      selMode =chart->GetSelectionMode();
      }
    else if(chartMatrix)
      {
      selMode = chartMatrix->GetSelectionMode();
      }
    if(selMode == this->SelectionMode)
      {
      this->parentAction()->blockSignals(true);
      this->parentAction()->setChecked(false);
      this->parentAction()->blockSignals(false);
      pqChartSelectionReaction::startSelection(this->View,
        vtkContextScene::SELECTION_NONE);
      }
    else if(selMode>=0)
      {
      pqChartSelectionReaction::startSelection(this->View, this->SelectionMode);
      }
    }
}
