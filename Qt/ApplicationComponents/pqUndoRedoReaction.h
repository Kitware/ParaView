/*=========================================================================

   Program: ParaView
   Module:    pqUndoRedoReaction.h

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

protected:
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
