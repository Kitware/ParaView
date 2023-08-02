// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditScalarBarReaction.h"

#include "pqDataRepresentation.h"
#include "pqProxyWidgetDialog.h"
#include "pqScalarBarVisibilityReaction.h"

//-----------------------------------------------------------------------------
pqEditScalarBarReaction::pqEditScalarBarReaction(QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
{
  QAction* tmp = new QAction(this);
  this->SBVReaction = new pqScalarBarVisibilityReaction(tmp, track_active_objects);
  this->connect(tmp, SIGNAL(changed()), SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqEditScalarBarReaction::~pqEditScalarBarReaction()
{
  delete this->SBVReaction;
}

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->SBVReaction->setRepresentation(repr);
}

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::updateEnableState()
{
  this->parentAction()->setEnabled(this->SBVReaction->parentAction()->isEnabled() &&
    this->SBVReaction->parentAction()->isChecked());
}

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::onTriggered()
{
  this->editScalarBar();
}

//-----------------------------------------------------------------------------
bool pqEditScalarBarReaction::editScalarBar()
{
  if (vtkSMProxy* sbProxy = this->SBVReaction->scalarBarProxy())
  {
    pqRepresentation* repr = this->SBVReaction->representation();

    pqProxyWidgetDialog dialog(sbProxy);
    dialog.setWindowTitle(tr("Edit Color Legend Properties"));
    dialog.setObjectName("ColorLegendEditor");
    dialog.setEnableSearchBar(true);
    dialog.setSettingsKey("ColorLegendEditor");

    repr->connect(&dialog, SIGNAL(accepted()), SLOT(renderViewEventually()));
    return dialog.exec() == QDialog::Accepted;
  }
  return false;
}
