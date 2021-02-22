/*=========================================================================

   Program: ParaView
   Module:    pqCameraUndoRedoReaction.cxx

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
