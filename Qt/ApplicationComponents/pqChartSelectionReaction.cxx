// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqChartSelectionReaction.h"

#include "pqContextView.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "pqSelectionManager.h"

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

  if (parentObject->data().isValid() &&
    parentObject->data().toInt() == pqChartSelectionReaction::CLEAR_SELECTION)
  {
    if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
    {
      this->connect(core->selectionManager(), SIGNAL(selectionChanged(pqOutputPort*)),
        SLOT(updateEnableState()));
      this->updateEnableState();
    }
  }
}

//-----------------------------------------------------------------------------
namespace
{
inline void setChartParameters(pqContextView* view, int selectionType, bool update_type,
  int selectionModifier, bool update_modifier)
{
  if (view != nullptr && !view->supportsSelection() && view->getContextViewProxy() == nullptr)
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

  if (update_modifier &&
    (selectionModifier < vtkContextScene::SELECTION_DEFAULT ||
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

    if (selectionType == pqChartSelectionReaction::CLEAR_SELECTION)
    {
      if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
        core->selectionManager()->clearSelection();
      }
    }
    else // Do selection
    {
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
}

//-----------------------------------------------------------------------------
int pqChartSelectionReaction::getSelectionModifier()
{
  int selectionModifier = this->Superclass::getSelectionModifier();
  switch (selectionModifier)
  {
    case (pqView::PV_SELECTION_ADDITION):
      return vtkContextScene::SELECTION_ADDITION;
      break;
    case (pqView::PV_SELECTION_SUBTRACTION):
      return vtkContextScene::SELECTION_SUBTRACTION;
      break;
    case (pqView::PV_SELECTION_TOGGLE):
      return vtkContextScene::SELECTION_TOGGLE;
      break;
    case (pqView::PV_SELECTION_DEFAULT):
    default:
      return vtkContextScene::SELECTION_DEFAULT;
      break;
  }
}

//-----------------------------------------------------------------------------
void pqChartSelectionReaction::updateEnableState()
{
  if (!this->View || !this->View->supportsSelection())
  {
    return;
  }

  this->stopSelection();
  auto parentAction = this->parentAction();
  if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
  {
    parentAction->setEnabled(core->selectionManager()->hasActiveSelection());
  }
  else
  {
    parentAction->setEnabled(false);
  }
}
