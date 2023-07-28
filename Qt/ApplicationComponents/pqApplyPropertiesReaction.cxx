// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqApplyPropertiesReaction.h"

#include "pqPropertiesPanel.h"

pqApplyPropertiesReaction::pqApplyPropertiesReaction(
  pqPropertiesPanel* panel, QAction* action, bool apply)
  : Superclass(action)
  , Panel(panel)
  , ShouldApply(apply)
{
  this->connect(panel, &pqPropertiesPanel::applyEnableStateChanged, this,
    &pqApplyPropertiesReaction::updateEnableState);
  this->updateEnableState();
}

void pqApplyPropertiesReaction::updateEnableState()
{
  if (this->ShouldApply)
  {
    this->parentAction()->setEnabled(this->Panel->canApply());
  }
  else
  {
    this->parentAction()->setEnabled(this->Panel->canReset());
  }
}

void pqApplyPropertiesReaction::onTriggered()
{
  if (this->ShouldApply)
  {
    this->Panel->apply();
  }
  else
  {
    this->Panel->reset();
  }
}
