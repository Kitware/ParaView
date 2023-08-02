// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqChangePipelineInputReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqChangeInputDialog.h"
#include "pqCoreUtilities.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqChangePipelineInputReaction::pqChangePipelineInputReaction(QAction* parentObject)
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
void pqChangePipelineInputReaction::updateEnableState()
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());
  if (filter == nullptr || filter->modifiedState() == pqProxy::UNINITIALIZED)
  {
    this->parentAction()->setEnabled(false);
    return;
  }

  this->parentAction()->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqChangePipelineInputReaction::changeInput()
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());

  if (!filter)
  {
    qCritical() << "No active filter.";
    return;
  }

  pqChangeInputDialog dialog(filter->getProxy(), pqCoreUtilities::mainWidget());
  dialog.setObjectName("ChangeInputDialog");
  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  BEGIN_UNDO_SET(tr("Change Input for %1").arg(filter->getSMName()));
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", filter->getProxy());

  const QMap<QString, QList<pqOutputPort*>> input_map = dialog.selectedInputs();
  QMap<QString, QList<pqOutputPort*>>::const_iterator iter;

  for (iter = input_map.begin(); iter != input_map.end(); iter++)
  {
    const QString& inputPortName = iter.key();
    const QList<pqOutputPort*>& inputs = iter.value();

    std::vector<vtkSMProxy*> inputPtrs;
    std::vector<unsigned int> inputPorts;

    Q_FOREACH (pqOutputPort* opport, inputs)
    {
      inputPtrs.push_back(opport->getSource()->getProxy());
      inputPorts.push_back(opport->getPortNumber());
    }

    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(inputPortName.toUtf8().data()));
    ip->SetProxies(static_cast<unsigned int>(inputPtrs.size()), &inputPtrs[0], &inputPorts[0]);
  }
  filter->getProxy()->UpdateVTKObjects();
  END_UNDO_SET();

  // render all views
  pqApplicationCore::instance()->render();
}
