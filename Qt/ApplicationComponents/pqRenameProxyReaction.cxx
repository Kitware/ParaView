// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRenameProxyReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqProxy.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

#include <QInputDialog>
#include <QString>
#include <QWidget>

//-----------------------------------------------------------------------------
pqRenameProxyReaction::pqRenameProxyReaction(
  QAction* renameAction, pqProxy* proxy, QWidget* parentWidget)
  : Superclass(renameAction)
  , Proxy(proxy)
  , ParentWidget(parentWidget)
{
}

//-----------------------------------------------------------------------------
pqRenameProxyReaction::pqRenameProxyReaction(QAction* renameAction, QWidget* parentWidget)
  : Superclass(renameAction)
  , Proxy(nullptr)
  , ParentWidget(parentWidget)
{
  this->connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)),
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqRenameProxyReaction::updateEnableState()
{
  this->Superclass::updateEnableState();
  this->parentAction()->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);
}

//-----------------------------------------------------------------------------
void pqRenameProxyReaction::onTriggered()
{
  auto proxy = (this->Proxy != nullptr) ? this->Proxy : pqActiveObjects::instance().activeSource();
  if (!proxy)
  {
    return;
  }

  bool ok;
  QString group = dynamic_cast<pqView*>(proxy) ? tr("View") : tr("Proxy");
  QString oldName = proxy->getSMName();
  QString newName = QInputDialog::getText(
    this->ParentWidget ? this->ParentWidget.data() : pqCoreUtilities::mainWidget(),
    tr("Rename") + " " + group + "...", tr("New name:"), QLineEdit::Normal, oldName, &ok);

  if (ok && !newName.isEmpty() && newName != oldName)
  {
    if (group == "View")
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("RenameView")
        .arg(newName.toUtf8().data())
        .arg(proxy->getProxy());
    }
    else
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("RenameProxy")
        .arg(proxy->getProxy())
        .arg(proxy->getSMGroup().toUtf8().data())
        .arg(newName.toUtf8().data());
    }
    BEGIN_UNDO_SET(tr("Rename") + " " + group);
    proxy->rename(newName);
    END_UNDO_SET();
  }
}
