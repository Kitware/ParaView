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
#include <QList>
#include <QMessageBox>
#include <QtDebug>

#include <vector>

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

  const bool retval = mbox.clickedButton() == mbox.button(QMessageBox::Yes);
  // cache response for future use.
  if (pqreader)
  {
    pqreader->setProperty("pqReloadFilesReaction::CachedState", retval);
  }
  return retval;
}
}

//-----------------------------------------------------------------------------
pqReloadFilesReaction::pqReloadFilesReaction(QAction* parentObject, ReloadModes reloadMode)
  : Superclass(parentObject)
  , ReloadMode(reloadMode)
{
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::sourceChanged, this,
    &pqReloadFilesReaction::updateEnableState);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqReloadFilesReaction::~pqReloadFilesReaction() = default;

//-----------------------------------------------------------------------------
void pqReloadFilesReaction::updateEnableState()
{
  std::vector<vtkSMSourceProxy*> sources;
  if (this->ReloadMode == ReloadModes::ActiveSource)
  {
    pqPipelineSource* source = pqActiveObjects::instance().activeSource();
    sources.push_back(source ? vtkSMSourceProxy::SafeDownCast(source->getProxy()) : nullptr);
  }
  else // ReloadMode == ReloadModes::AllSources
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqPipelineSource*> allSources = smmodel->findItems<pqPipelineSource*>();
    for (int cc = 0; cc < allSources.size(); cc++)
    {
      sources.push_back(vtkSMSourceProxy::SafeDownCast(allSources[cc]->getProxy()));
    }
  }

  bool enabled = false;
  vtkNew<vtkSMReaderReloadHelper> helper;
  for (const auto& source : sources)
  {
    if (helper->SupportsReload(source))
    {
      enabled = true;
      break;
    }
  }
  this->parentAction()->setEnabled(enabled);
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
bool pqReloadFilesReaction::reloadSources()
{
  std::vector<vtkSMSourceProxy*> sources;
  if (this->ReloadMode == ReloadModes::ActiveSource)
  {
    pqPipelineSource* source = pqActiveObjects::instance().activeSource();
    sources.push_back(source ? vtkSMSourceProxy::SafeDownCast(source->getProxy()) : nullptr);
  }
  else // ReloadMode == ReloadModes::AllSources
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqPipelineSource*> allSources = smmodel->findItems<pqPipelineSource*>();
    for (int cc = 0; cc < allSources.size(); cc++)
    {
      sources.push_back(vtkSMSourceProxy::SafeDownCast(allSources[cc]->getProxy()));
    }
  }
  bool success = false;
  for (const auto& source : sources)
  {
    success |= pqReloadFilesReaction::reload(source);
  }
  return success;
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
