// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqShowHideAllReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMViewProxy.h"

//-----------------------------------------------------------------------------
pqShowHideAllReaction::pqShowHideAllReaction(QAction* parentObject, ActionType action)
  : Superclass(parentObject)
  , Action(action)
{
}

//-----------------------------------------------------------------------------
void pqShowHideAllReaction::act(ActionType action)
{
  pqView* activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (action == ActionType::Show)
  {
    BEGIN_UNDO_SET(tr("Show All"));
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    controller->ShowAll(viewProxy);
    END_UNDO_SET();
  }
  else
  {
    BEGIN_UNDO_SET(tr("Hide All"));
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    controller->HideAll(viewProxy);
    END_UNDO_SET();
  }
  pqApplicationCore::instance()->render();
}
