/*=========================================================================

   Program: ParaView
   Module:    pqGenericSelectionReaction.h

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
#ifndef __pqGenericSelectionReaction_h 
#define __pqGenericSelectionReaction_h

#include "pqReaction.h"
#include <QPointer> // needed for QPointer.

class QActionGroup;

/// @ingroup Reactions
/// Reaction for creating selections on chart views.
class PQAPPLICATIONCOMPONENTS_EXPORT pqGenericSelectionReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;
public:
  /// Constructor.\c modifierGroup is used to determine selection modifier. If
  /// there's a non-null checkedAction() in the group, we use that action's
  /// data() to determine the selection mode e.g.
  /// vtkContextScene::SELECTION_ADDITION,
  /// vtkContextScene::SELECTION_SUBTRACTION etc. If no QActionGroup is
  /// specified or no checked action is present, then the default mode of
  /// vtkContextScene::SELECTION_DEFAULT is used.
  pqGenericSelectionReaction(QAction* parent, QActionGroup* modifierGroup = NULL);

protected slots:
  /// called when modifier group is changed.
  virtual void modifiersChanged() {}

protected:
  /// Get the current state of selection modifier, if any
  virtual int getSelectionModifier();

  /// Uncheck selection modifiers, if any
  virtual void uncheckSelectionModifiers();

  /// Disable/Enable selection modifiers, if any
  virtual void disableSelectionModifiers(bool disable);

  QPointer<QActionGroup> ModifierGroup;

private:
  Q_DISABLE_COPY(pqGenericSelectionReaction)
};

#endif
