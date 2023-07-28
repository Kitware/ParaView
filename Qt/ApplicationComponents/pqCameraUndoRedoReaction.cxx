// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraUndoRedoReaction.h"

#include "pqActiveObjects.h"
#include "pqView.h"

//-----------------------------------------------------------------------------
pqCameraUndoRedoReaction::pqCameraUndoRedoReaction(
  QAction* parentObject, bool undo_mode, pqView* view)
  : Superclass(parentObject)
{
  this->Undo = undo_mode;

  if (view)
  {
    this->setActiveView(view);
  }
  else
  {
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
      SLOT(setActiveView(pqView*)));
    this->setActiveView(pqActiveObjects::instance().activeView());
  }
}

//-----------------------------------------------------------------------------
void pqCameraUndoRedoReaction::undo(pqView* view)
{
  if (!view)
  {
    return;
  }
  view->undo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqCameraUndoRedoReaction::redo(pqView* view)
{
  if (!view)
  {
    return;
  }
  view->redo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqCameraUndoRedoReaction::setActiveView(pqView* view)
{
  if (this->LastView)
  {
    QObject::disconnect(this->LastView, nullptr, this, nullptr);
    this->LastView = nullptr;
  }

  if (!view || !view->supportsUndo())
  {
    this->setEnabled(false);
    return;
  }

  this->LastView = view;

  if (this->Undo)
  {
    this->setEnabled(view->canUndo());
    QObject::connect(view, SIGNAL(canUndoChanged(bool)), this, SLOT(setEnabled(bool)));
  }
  else
  {
    this->setEnabled(view->canRedo());
    QObject::connect(view, SIGNAL(canRedoChanged(bool)), this, SLOT(setEnabled(bool)));
  }
}

//-----------------------------------------------------------------------------
void pqCameraUndoRedoReaction::onTriggered()
{
  if (this->Undo)
  {
    pqCameraUndoRedoReaction::undo(this->LastView);
  }
  else
  {
    pqCameraUndoRedoReaction::redo(this->LastView);
  }
}
