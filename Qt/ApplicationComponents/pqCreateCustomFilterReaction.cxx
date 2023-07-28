// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  this->parentAction()->setEnabled(!pqActiveObjects::instance().selection().empty());
}

//-----------------------------------------------------------------------------
void pqCreateCustomFilterReaction::createCustomFilter()
{
  // Get the selected sources from the application core. Notify the user
  // if the selection is empty.
  QWidget* mainWin = pqCoreUtilities::mainWidget();

  if (pqActiveObjects::instance().selection().empty())
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
    QMessageBox::warning(mainWin, tr("Create Custom Filter Error"),
      tr("The selected objects cannot be used to make a custom filter.\n"
         "To create a new custom filter, select the sources and "
         "filters you want.\nThen, launch the creation wizard."),
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
