// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqChartSelectionReaction_h
#define pqChartSelectionReaction_h

#include "pqSelectionReaction.h"

#include "vtkChart.h" // for vtkChart::ACTION_TYPES_COUNT

#include <QPointer> // for QPointer

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
   * ParaView-specific selection modes
   */
  enum SelectionMode
  {
    CLEAR_SELECTION = vtkChart::ACTION_TYPES_COUNT
  };

  /**
   * Constructor. \c parent is expected to have data() that indicates the
   * selection type e.g. vtkChart::SELECT_RECTANGLE or vtkChart::SELECT_POLYGON.
   * One can also use vtkChart::CLEAR_SELECTION to clear the current selection.
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
   * Start selection on the view where:
   * - selectionType is one of vtkChart::SELECT_POLYGON, vtkChart::SELECT_RECTANGLE, etc. or
   * pqChartSelectionReaction::CLEAR_SELECTION
   * - selectionModifier is one of vtkContextScene::SELECTION_DEFAULT,
   * vtkContextScene::SELECTION_ADDITION, etc.
   */
  static void startSelection(pqContextView* view, int selectionType, int selectionModifier);

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  virtual void triggered(bool);

  /**
   * Stops selecting on the view.
   */
  void stopSelection();

  /**
   * Called when modifier group is changed.
   */
  void modifiersChanged() override;

  /**
   * Get the current state of selection modifier, converting it to vtkScene enum.
   */
  int getSelectionModifier() override;

  /**
   * Handles enable state for `CLEAR_SELECTION` action.
   */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqChartSelectionReaction)
  QPointer<pqContextView> View;
};

#endif
