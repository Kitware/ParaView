// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLinkSelectionReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLinksModel.h"
#include "pqSelectionLinkDialog.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "vtkSMSourceProxy.h"

#include <QSet>

//-----------------------------------------------------------------------------
pqLinkSelectionReaction::pqLinkSelectionReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));

  // nameChanged() is fired even when modified state is changed ;).
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqLinkSelectionReaction::updateEnableState()
{
  pqSelectionManager* selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));
  if (selectionManager)
  {
    pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
    if (activeSource != nullptr && selectionManager->hasActiveSelection())
    {
      Q_FOREACH (pqOutputPort* port, selectionManager->getSelectedPorts())
      {
        if (port->getSource() == activeSource)
        {
          this->parentAction()->setEnabled(false);
          return;
        }
      }
      this->parentAction()->setEnabled(true);
      return;
    }
  }
  this->parentAction()->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqLinkSelectionReaction::linkSelection()
{
  pqSelectionLinkDialog slDialog(pqCoreUtilities::mainWidget());
  if (slDialog.exec() != QDialog::Accepted)
  {
    return;
  }

  pqSelectionManager* selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));

  int index = 0;
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QString name = QString("SelectionLink%1").arg(index);
  while (model->getLink(name))
  {
    name = QString("SelectionLink%1").arg(++index);
  }

  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  model->addSelectionLink(name, selectionManager->getSelectedPort()->getSourceProxy(),
    activeSource->getProxy(), slDialog.convertToIndices());
}
