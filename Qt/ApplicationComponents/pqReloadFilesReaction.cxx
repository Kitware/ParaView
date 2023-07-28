// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqReloadFilesReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkSMReaderReloadHelper.h"
#include "vtkSMSourceProxy.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QMessageBox>
#include <QtDebug>

namespace
{
bool PromptForNewFiles(vtkSMSourceProxy* reader)
{
  pqProxy* pqreader =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqProxy*>(reader);
  if (pqreader && pqreader->property("pqReloadFilesReaction::CachedState").isValid())
  {
    return pqreader->property("pqReloadFilesReaction::CachedState").toBool();
  }

  QMessageBox mbox(QMessageBox::Question,
    QCoreApplication::translate("pqReloadFilesReaction", "Reload Options"),
    QCoreApplication::translate("pqReloadFilesReaction",
      "This reader supports file series. Do you want to look for new files "
      "in the series and load those, or reload the existing files?"),
    QMessageBox::Yes | QMessageBox::No, pqCoreUtilities::mainWidget());
  mbox.setObjectName("reloadOptionsMessageBox");
  mbox.button(QMessageBox::Yes)->setObjectName("findNewFilesButton");
  mbox.button(QMessageBox::Yes)
    ->setText(QCoreApplication::translate("pqReloadFilesReaction", "Find new files"));

  mbox.button(QMessageBox::No)->setObjectName("reloadExistingButton");
  mbox.button(QMessageBox::No)
    ->setText(QCoreApplication::translate("pqReloadFilesReaction", "Reload existing file(s)"));
  mbox.exec();

  bool retval = mbox.clickedButton() == mbox.button(QMessageBox::Yes);
  // cache response for future use.
  if (pqreader)
  {
    pqreader->setProperty("pqReloadFilesReaction::CachedState", retval);
  }
  return retval;
}
}

//-----------------------------------------------------------------------------
pqReloadFilesReaction::pqReloadFilesReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)),
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqReloadFilesReaction::~pqReloadFilesReaction() = default;

//-----------------------------------------------------------------------------
void pqReloadFilesReaction::updateEnableState()
{
  vtkSMProxy* source = pqActiveObjects::instance().activeSource()
    ? pqActiveObjects::instance().activeSource()->getProxy()
    : nullptr;

  vtkNew<vtkSMReaderReloadHelper> helper;
  this->parentAction()->setEnabled(helper->SupportsReload(vtkSMSourceProxy::SafeDownCast(source)));
}

//-----------------------------------------------------------------------------
bool pqReloadFilesReaction::reload()
{
  vtkSMProxy* source = pqActiveObjects::instance().activeSource()
    ? pqActiveObjects::instance().activeSource()->getProxy()
    : nullptr;
  return pqReloadFilesReaction::reload(vtkSMSourceProxy::SafeDownCast(source));
}

//-----------------------------------------------------------------------------
bool pqReloadFilesReaction::reload(vtkSMSourceProxy* proxy)
{
  vtkNew<vtkSMReaderReloadHelper> helper;
  if (!helper->SupportsReload(proxy))
  {
    return false;
  }

  BEGIN_UNDO_EXCLUDE();
  if (helper->SupportsFileSeries(proxy) && PromptForNewFiles(proxy))
  {
    helper->ExtendFileSeries(proxy);
  }
  else
  {
    helper->ReloadFiles(proxy);
  }
  pqApplicationCore::instance()->render();
  END_UNDO_EXCLUDE();
  return true;
}
