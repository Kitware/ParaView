/*=========================================================================

  Program: ParaView
  Module:    pqShowHideAllReaction.cxx

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
  vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : NULL;
  if (action == ActionType::Show)
  {
    BEGIN_UNDO_SET("Show All");
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    controller->ShowAll(viewProxy);
    END_UNDO_SET();
  }
  else
  {
    BEGIN_UNDO_SET("Hide All");
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    controller->HideAll(viewProxy);
    END_UNDO_SET();
  }
  pqApplicationCore::instance()->render();
}
