// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraUndoRedoReaction_h
#define pqCameraUndoRedoReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqView;

/**
 * @ingroup Reactions
 * Reaction for camera undo or redo.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCameraUndoRedoReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor parent cannot be nullptr. When undo is true, acts as
   * undo-reaction, else acts as redo reaction.
   * If \c view ==nullptr then active view is used.
   */
  pqCameraUndoRedoReaction(QAction* parent, bool undo, pqView* view = nullptr);

  /**
   * undo.
   */
  static void undo(pqView* view);

  /**
   * redo.
   */
  static void redo(pqView* view);

protected Q_SLOTS:
  void setEnabled(bool enable) { this->parentAction()->setEnabled(enable); }
  void setActiveView(pqView*);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqCameraUndoRedoReaction)
  QPointer<pqView> LastView;
  bool Undo;
};

#endif
