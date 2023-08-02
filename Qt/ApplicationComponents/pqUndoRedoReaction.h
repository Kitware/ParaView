// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqUndoRedoReaction_h
#define pqUndoRedoReaction_h

#include "pqReaction.h"

class pqUndoStack;

/**
 * @ingroup Reactions
 * Reaction for application undo-redo.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqUndoRedoReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * if \c undo is set to true, then this behaves as an undo-reaction otherwise
   * as a redo-reaction.
   */
  pqUndoRedoReaction(QAction* parent, bool undo);

  /**
   * undo.
   */
  static void undo();

  /**
   * redo.
   */
  static void redo();

  /**
   * Clear stack
   */
  static void clear();

protected Q_SLOTS:
  void enable(bool);
  void setLabel(const QString& label);
  void setUndoStack(pqUndoStack*);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override
  {
    if (this->Undo)
    {
      pqUndoRedoReaction::undo();
    }
    else
    {
      pqUndoRedoReaction::redo();
    }
  }

private:
  Q_DISABLE_COPY(pqUndoRedoReaction)

  bool Undo;
};

#endif
