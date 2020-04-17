/*=========================================================================

   Program: ParaView
   Module:    pqChartSelectionReaction.h

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
#ifndef pqChartSelectionReaction_h
#define pqChartSelectionReaction_h

#include "pqSelectionReaction.h"
#include <QPointer> // needed for QPointer.

class pqContextView;
class vtkObject;

/**
* @ingroup Reactions
* Reaction for creating selections on chart views.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqChartSelectionReaction : public pqSelectionReaction
{
  Q_OBJECT
  typedef pqSelectionReaction Superclass;

public:
  /**
  * Constructor. \c parent is expected to have data() that indicates the
  * selection type e.g. vtkChart::SELECT_RECTANGLE or vtkChart::SELECT_POLYGON.
  * QActionGroup \c modifierGroup is used to determine selection modifier. If
  * there's a non-null checkedAction() in the group, we use that action's
  * data() to determine the selection mode e.g.
  * vtkContextScene::SELECTION_ADDITION,
  * vtkContextScene::SELECTION_SUBTRACTION etc. If no QActionGroup is
  * specified or no checked action is present, then the default mode of
  * vtkContextScene::SELECTION_DEFAULT is used.
  */
  pqChartSelectionReaction(QAction* parent, pqContextView* view, QActionGroup* modifierGroup);

  /**
  * start selection on the view where selectionType is one of
  * vtkChart::SELECT_POLYGON, vtkChart::SELECT_RECTANGLE, etc., and
  * selectionModifier is one of vtkContextScene::SELECTION_DEFAULT,
  * vtkContextScene::SELECTION_ADDITION, etc.
  */
  static void startSelection(pqContextView* view, int selectionType, int selectionModifier);

protected Q_SLOTS:
  /**
  * Called when the action is triggered.
  */
  virtual void triggered(bool);

  /**
  * stops selecting on the view
  */
  void stopSelection();

  /**
  * called when modifier group is changed.
  */
  void modifiersChanged() override;

  // Get the current state of selection modifier, converting it to vtkScene enum
  int getSelectionModifier() override;

private:
  Q_DISABLE_COPY(pqChartSelectionReaction)
  QPointer<pqContextView> View;
};

#endif
