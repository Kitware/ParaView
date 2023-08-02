// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUndoRedoReaction.h"

#include "pqApplicationCore.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqUndoRedoReaction::pqUndoRedoReaction(QAction* parentObject, bool _undo)
  : Superclass(parentObject)
{
  this->Undo = _undo;
  this->enable(false);

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (!stack)
  {
    QObject::connect(pqApplicationCore::instance(), SIGNAL(undoStackChanged(pqUndoStack*)), this,
      SLOT(setUndoStack(pqUndoStack*)));
  }
  else
  {
    this->setUndoStack(stack);
  }
}

//-----------------------------------------------------------------------------
void pqUndoRedoReaction::setUndoStack(pqUndoStack* stack)
{
  if (this->Undo)
  {
    QObject::connect(stack, SIGNAL(canUndoChanged(bool)), this, SLOT(enable(bool)));
    QObject::connect(
      stack, SIGNAL(undoLabelChanged(const QString&)), this, SLOT(setLabel(const QString&)));
  }
  else
  {
    QObject::connect(stack, SIGNAL(canRedoChanged(bool)), this, SLOT(enable(bool)));
    QObject::connect(
      stack, SIGNAL(redoLabelChanged(const QString&)), this, SLOT(setLabel(const QString&)));
  }
}

//-----------------------------------------------------------------------------
void pqUndoRedoReaction::undo()
{
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (!stack)
  {
    qCritical("No application wide undo stack.");
    return;
  }
  stack->undo();
}

//-----------------------------------------------------------------------------
void pqUndoRedoReaction::redo()
{
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (!stack)
  {
    qCritical("No application wide undo stack.");
    return;
  }
  stack->redo();
}
//-----------------------------------------------------------------------------
void pqUndoRedoReaction::clear()
{
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (!stack)
  {
    qCritical("No application wide undo stack.");
    return;
  }
  stack->clear();
}

//-----------------------------------------------------------------------------
void pqUndoRedoReaction::enable(bool can_undo)
{
  this->parentAction()->setEnabled(can_undo);
}

//-----------------------------------------------------------------------------
void pqUndoRedoReaction::setLabel(const QString& label)
{
  if (this->Undo)
  {
    this->parentAction()->setText(label.isEmpty() ? tr("Can't Undo") : (tr("&Undo") + " " + label));
    this->parentAction()->setStatusTip(
      label.isEmpty() ? tr("Can't Undo") : (tr("Undo") + " " + label));
  }
  else
  {
    this->parentAction()->setText(label.isEmpty() ? tr("Can't Redo") : (tr("&Redo") + " " + label));
    this->parentAction()->setStatusTip(
      label.isEmpty() ? tr("Can't Redo") : (tr("Redo") + " " + label));
  }
}
