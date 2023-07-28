// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAutoApplyReaction.h"

#include "pqCoreUtilities.h"
#include "vtkCommand.h"
#include "vtkPVGeneralSettings.h"

//-----------------------------------------------------------------------------
pqAutoApplyReaction::pqAutoApplyReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  parentObject->setChecked(this->autoApply());
  QObject::connect(parentObject, SIGNAL(triggered(bool)), this, SLOT(checkStateChanged(bool)));

  vtkPVGeneralSettings* gs = vtkPVGeneralSettings::GetInstance();
  pqCoreUtilities::connect(gs, vtkCommand::ModifiedEvent, this, SLOT(updateState()));
}

//-----------------------------------------------------------------------------
void pqAutoApplyReaction::checkStateChanged(bool autoAccept)
{
  pqAutoApplyReaction::setAutoApply(autoAccept);
}

//-----------------------------------------------------------------------------
bool pqAutoApplyReaction::autoApply()
{
  return vtkPVGeneralSettings::GetInstance()->GetAutoApply();
}

//-----------------------------------------------------------------------------
void pqAutoApplyReaction::setAutoApply(bool autoAccept)
{
  vtkPVGeneralSettings::GetInstance()->SetAutoApply(autoAccept);
  // FIXME: How should this change be saved in vtkSMSettings?
}

//-----------------------------------------------------------------------------
void pqAutoApplyReaction::updateState()
{
  vtkPVGeneralSettings* gs = vtkPVGeneralSettings::GetInstance();
  this->parentAction()->setChecked(gs->GetAutoApply());
}
