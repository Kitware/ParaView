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

#include "pqContextView.h"
#include "pqCoreUtilities.h"
#include "vtkChart.h"
#include "vtkCommand.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMContextViewProxy.h"
#include "vtkScatterPlotMatrix.h"

#include <QActionGroup>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqChartSelectionReaction::pqChartSelectionReaction(
  QAction* parentObject, pqContextView* view, QActionGroup* modifierGroup)
  : Superclass(parentObject, modifierGroup)
  , View(view)
{
  parentObject->setEnabled(view != nullptr && view->supportsSelection());
  this->connect(parentObject, SIGNAL(triggered(bool)), SLOT(triggered(bool)));

  vtkRenderWindowInteractor* interactor =
    (view ? view->getVTKContextView()->GetInteractor() : nullptr);
  if (interactor)
  {
    pqCoreUtilities::connect(
      interactor, vtkCommand::LeftButtonReleaseEvent, this, SLOT(stopSelection()));
  }
}

//-----------------------------------------------------------------------------
inline void setChartParameters(pqContextView* view, int selectionType, bool update_type,
  int selectionModifier, bool update_modifier)
{
  if (view == nullptr && !view->supportsSelection() && view->getContextViewProxy() == nullptr)
  {
    return;
  }

  vtkAbstractContextItem* contextItem = view->getContextViewProxy()->GetContextItem();
  vtkChart* chart = vtkChart::SafeDownCast(contextItem);
  vtkScatterPlotMatrix* chartMatrix = vtkScatterPlotMatrix::SafeDownCast(contextItem);
  if (chartMatrix)
  {
    chart = chartMatrix->GetMainChart();
  }

  if (update_modifier && (selectionModifier < vtkContextScene::SELECTION_NONE ||
                           selectionModifier > vtkContextScene::SELECTION_TOGGLE))
  {
    qWarning() << "Invalid selection modifier  " << selectionModifier
               << ", using vtkContextScene::SELECTION_DEFAULT";
    selectionModifier = vtkContextScene::SELECTION_DEFAULT;
  }

  if (chart)
  {
    if (update_type)
    {
      chart->SetActionToButton(selectionType, vtkContextMouseEvent::LEFT_BUTTON);
    }
    if (update_modifier)
    {
      chart->SetSelectionMode(selectionModifier);
    }
  }
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::startSelection(
  pqContextView* view, int selectionType, int selectionModifier)
{
  ::setChartParameters(view, selectionType, true, selectionModifier, true);
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::stopSelection()
{
  ::setChartParameters(this->View, vtkChart::PAN, true, vtkContextScene::SELECTION_DEFAULT, true);
  this->parentAction()->setChecked(false);
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::modifiersChanged()
{
  int selectionModifier = this->getSelectionModifier();
  ::setChartParameters(this->View, -1, false, selectionModifier, true);
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::triggered(bool checked)
{
  if (this->View && this->View->supportsSelection() && this->View->getContextViewProxy())
  {
    QAction* _action = this->parentAction();
    int selectionType = vtkChart::SELECT_RECTANGLE;
    if (_action->data().isValid())
    {
      selectionType = _action->data().toInt();
    }

    int selectionModifier = this->getSelectionModifier();
    if (checked)
    {
      pqChartSelectionReaction::startSelection(this->View, selectionType, selectionModifier);
    }
    else
    {
      pqChartSelectionReaction::stopSelection();
    }
  }
}

//-----------------------------------------------------------------------------
int pqChartSelectionReaction::getSelectionModifier()
{
  int selectionModifier = this->Superclass::getSelectionModifier();
  switch (selectionModifier)
  {
    case (pqView::PV_SELECTION_DEFAULT):
      return vtkContextScene::SELECTION_DEFAULT;
      break;
    case (pqView::PV_SELECTION_ADDITION):
      return vtkContextScene::SELECTION_ADDITION;
      break;
    case (pqView::PV_SELECTION_SUBTRACTION):
      return vtkContextScene::SELECTION_SUBTRACTION;
      break;
    case (pqView::PV_SELECTION_TOGGLE):
      return vtkContextScene::SELECTION_TOGGLE;
      break;
    default:
      return vtkContextScene::SELECTION_NONE;
      break;
  }
}
