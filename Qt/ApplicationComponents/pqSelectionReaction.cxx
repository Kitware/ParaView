// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSelectionReaction.h"
#include "pqView.h"
#include <QActionGroup>

//-----------------------------------------------------------------------------
int pqSelectionReaction::getSelectionModifier()
{
  if (this->ModifierGroup)
  {
    // we cannot use QActionGroup::checkedAction() since the ModifierGroup may
    // not be exclusive.
    Q_FOREACH (QAction* maction, this->ModifierGroup->actions())
    {
      if (maction->isChecked() && maction->data().isValid())
      {
        return maction->data().toInt();
      }
    }
  }
  return pqView::PV_SELECTION_DEFAULT;
}

void pqSelectionReaction::uncheckSelectionModifiers()
{
  if (this->ModifierGroup)
  {
    Q_FOREACH (QAction* act, this->ModifierGroup->actions())
    {
      act->setChecked(false);
    }
  }
}

void pqSelectionReaction::disableSelectionModifiers(bool disable)
{
  if (this->ModifierGroup)
  {
    this->ModifierGroup->setEnabled(!disable);
    if (disable)
    {
      this->uncheckSelectionModifiers();
    }
  }
}
//-----------------------------------------------------------------------------
pqSelectionReaction::pqSelectionReaction(QAction* parentObject, QActionGroup* modifierGroup)
  : Superclass(parentObject)
  , ModifierGroup(modifierGroup)
{
  // if modified is changed while selection is in progress, we need to ensure we
  // update the selection modifier on the view.
  if (modifierGroup)
  {
    this->connect(modifierGroup, SIGNAL(triggered(QAction*)), SLOT(modifiersChanged()));
  }
}
