/*=========================================================================

   Program: ParaView
   Module:  pqEditScalarBarReaction.cxx

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
    dialog.setWindowTitle("Edit Color Legend Properties");
    dialog.setObjectName("ColorLegendEditor");
    dialog.setEnableSearchBar(true);
    dialog.setSettingsKey("ColorLegendEditor");

    repr->connect(&dialog, SIGNAL(accepted()), SLOT(renderViewEventually()));
    return dialog.exec() == QDialog::Accepted;
  }
  return false;
}
