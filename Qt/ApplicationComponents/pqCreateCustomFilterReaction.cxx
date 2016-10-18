/*=========================================================================

   Program: ParaView
   Module:    pqCreateCustomFilterReaction.cxx

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
#include "pqCreateCustomFilterReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqCustomFilterDefinitionModel.h"
#include "pqCustomFilterDefinitionWizard.h"

#include <QDebug>
#include <QMessageBox>

//-----------------------------------------------------------------------------
pqCreateCustomFilterReaction::pqCreateCustomFilterReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)), this,
    SLOT(updateEnableState()));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(selectionChanged(const pqProxySelection&)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqCreateCustomFilterReaction::updateEnableState()
{
  this->parentAction()->setEnabled(pqActiveObjects::instance().selection().size() > 0);
}

//-----------------------------------------------------------------------------
void pqCreateCustomFilterReaction::createCustomFilter()
{
  // Get the selected sources from the application core. Notify the user
  // if the selection is empty.
  QWidget* mainWin = pqCoreUtilities::mainWidget();

  if (pqActiveObjects::instance().selection().size() == 0)
  {
    qCritical() << "No pipeline objects are selected."
                   "To create a new custom filter, select the sources and "
                   "filters you want. Then, launch the creation wizard.";
    return;
  }

  // Create a custom filter definition model with the pipeline
  // selection. The model only accepts pipeline sources. Notify the
  // user if the model is empty.
  pqCustomFilterDefinitionModel custom;
  custom.setContents(pqActiveObjects::instance().selection());
  if (!custom.hasChildren(QModelIndex()))
  {
    QMessageBox::warning(mainWin, "Create Custom Filter Error",
      "The selected objects cannot be used to make a custom filter.\n"
      "To create a new custom filter, select the sources and "
      "filters you want.\nThen, launch the creation wizard.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  pqCustomFilterDefinitionWizard wizard(&custom, mainWin);
  if (wizard.exec() == QDialog::Accepted)
  {
    // Create a new compound proxy from the custom filter definition.
    wizard.createCustomFilter();
    // QString customName = wizard.getCustomFilterName();
    // Not sure I want this anymore.
    // // Launch the custom filter manager in case the user wants to save
    // // the compound proxy definition. Select the new custom filter for
    // // the user.
    // this->onToolsManageCustomFilters();
    // this->Implementation->CustomFilterManager->selectCustomFilter(customName);
  }
}
