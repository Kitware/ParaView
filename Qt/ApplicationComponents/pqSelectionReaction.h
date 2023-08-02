// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionReaction_h
#define pqSelectionReaction_h

#include "pqReaction.h"
#include <QPointer> // needed for QPointer.

class QActionGroup;

/**
 * @ingroup Reactions
 * Generric reaction for creating selections on views.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSelectionReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor.\c modifierGroup is used to determine selection modifier. If
   * there's a non-nullptr checkedAction() in the group, we use that action's
   * data() to determine the selection mode e.g.
   * pqView::PVSELECTION_ADDITION,
   * pqView::PVSELECTION_SUBTRACTION etc. If no QActionGroup is
   * specified or no checked action is present, then the default mode of
   * pqView::PVSELECTION_DEFAULT is used.
   */
  pqSelectionReaction(QAction* parent, QActionGroup* modifierGroup = nullptr);

protected Q_SLOTS:
  /**
   * called when modifier group is changed.
   */
  virtual void modifiersChanged() {}

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Get the current state of selection modifier, if any
   */
  virtual int getSelectionModifier();

  /**
   * Uncheck selection modifiers, if any
   */
  virtual void uncheckSelectionModifiers();

  /**
   * Disable/Enable selection modifiers, if any
   */
  virtual void disableSelectionModifiers(bool disable);

  QPointer<QActionGroup> ModifierGroup;

private:
  Q_DISABLE_COPY(pqSelectionReaction)
};

#endif
