// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqChangeFileNameReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqUndoStack.h"

#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include <vtksys/SystemTools.hxx>

#include <QInputDialog>
#include <QMessageBox>

#include <algorithm>
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

//-----------------------------------------------------------------------------
pqChangeFileNameReaction::pqChangeFileNameReaction(QAction* parentObject)
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
void pqChangeFileNameReaction::updateEnableState()
{
  pqPipelineSource* source = (pqActiveObjects::instance().activeSource());
  if (source)
  {
    vtkSMSourceProxy* proxy = source->getSourceProxy();
    if (proxy && (proxy->GetProperty("FileName") || proxy->GetProperty("FileNames")))
    {
      this->parentAction()->setEnabled(true);
      return;
    }
  }
  this->parentAction()->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqChangeFileNameReaction::changeFileName()
{
  pqPipelineSource* source = (pqActiveObjects::instance().activeSource());
  if (!source)
  {
    return;
  }

  vtkSMSourceProxy* proxy = source->getSourceProxy();
  if (!proxy)
  {
    return;
  }

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return;
  }

  vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  if (!readerFactory)
  {
    return;
  }

  const auto& filtersDetailed = readerFactory->GetSupportedFileTypesDetailed(server->session());

  const auto& currentReaderName = proxy->GetXMLName();

  const auto& currentReaderMatch =
    std::count_if(std::begin(filtersDetailed), std::end(filtersDetailed),
      [currentReaderName](const FileTypeDetailed& ftd) { return ftd.Name == currentReaderName; });

  if (currentReaderMatch == 0)
  {
    QString warningTitle(tr("Change File operation aborted"));
    QMessageBox::warning(pqCoreUtilities::mainWidget(), warningTitle,
      tr("No reader associated to the selected source was found!"), QMessageBox::Ok);
    return;
  }
  if (currentReaderMatch > 1)
  {
    QString warningTitle(tr("Change File operation aborted"));
    QMessageBox::warning(pqCoreUtilities::mainWidget(), warningTitle,
      tr("More than one reader associated to the selected source were found!"), QMessageBox::Ok);
    return;
  }

  const auto& currentReaderFound =
    std::find_if(std::begin(filtersDetailed), std::end(filtersDetailed),
      [currentReaderName](const FileTypeDetailed& ftd) { return ftd.Name == currentReaderName; });

  // In theory it should never append as we checked exactly one reader exists
  assert(currentReaderFound != std::end(filtersDetailed) &&
    "No current reader found when exaclty one exists!");

  const auto& extensionList = currentReaderFound->FilenamePatterns;

  QString qExtensions = QString("Supported files (");
  for (const std::string& extension : extensionList)
  {
    qExtensions += QString(extension.c_str()) + QString(" ");
  }
  qExtensions += QString(")");

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), QString(), QString(), qExtensions, true);

  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFilesAndDirectories);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    QList<QStringList> allFiles = fileDialog.getAllSelectedFiles();
    std::vector<std::string> allFilesStd{};
    for (const auto& currentFiles : allFiles)
    {
      for (const auto& file : currentFiles)
      {
        allFilesStd.push_back(file.toStdString());
      }
    }

    for (const auto propertyName : { "FileName", "FileNames" })
    {
      if (vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(propertyName)))
      {
        SM_SCOPED_TRACE(CallFunction)
          .arg("ReplaceReaderFileName")
          .arg(proxy)
          .arg(allFilesStd)
          .arg(propertyName);
        vtkSMCoreUtilities::ReplaceReaderFileName(proxy, allFilesStd, propertyName);
        CLEAR_UNDO_STACK();
        break;
      }
    }
  }
}
