/*=========================================================================

   Program: ParaView
   Module:    pqCameraUndoRedoReaction.h

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
  pqCameraUndoRedoReaction(QAction* parent, bool undo, pqView* view = 0);

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

protected:
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
