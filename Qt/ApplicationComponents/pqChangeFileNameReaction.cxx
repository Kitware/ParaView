/*=========================================================================

   Program: ParaView
   Module:    pqChangeFileNameReaction.cxx

   Copyright (c) Kitware, Inc.
   All rights reserved.
   See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

========================================================================*/
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
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include <vtksys/SystemTools.hxx>

#include <QInputDialog>

#include <algorithm>
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

  vtkNew<vtkCollection> collection;
  vtkPVXMLElement* hints = proxy->GetHints();
  hints->FindNestedElementByName("ReaderFactory", collection);

  if (!collection->GetNumberOfItems())
  {
    return;
  }

  vtkPVXMLElement* readerFactory = vtkPVXMLElement::SafeDownCast(collection->GetItemAsObject(0));

  if (!readerFactory)
  {
    return;
  }

  const char* extensions = readerFactory->GetAttribute("extensions");
  if (!extensions)
  {
    qWarning("Extensions are not specified for the reader. Ignoring File Name Change Request");
    return;
  }

  std::vector<std::string> extensionList;
  vtksys::SystemTools::Split(extensions, extensionList, ' ');

  QString qExtensions = QString("Supported files (");
  for (const std::string& extension : extensionList)
  {
    qExtensions += QString("*.") + QString(extension.c_str()) + QString(" ");
  }
  qExtensions += QString(")");

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Open File:"), QString(), qExtensions);

  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    QStringList files = fileDialog.getSelectedFiles();
    std::vector<std::string> filesStd(files.size());
    std::transform(files.constBegin(), files.constEnd(), filesStd.begin(),
      [](const QString& str) -> std::string { return str.toStdString(); });

    for (const auto propertyName : { "FileName", "FileNames" })
    {
      if (vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(propertyName)))
      {
        SM_SCOPED_TRACE(CallFunction)
          .arg("ReplaceReaderFileName")
          .arg(proxy)
          .arg(filesStd)
          .arg(propertyName);
        vtkSMCoreUtilities::ReplaceReaderFileName(proxy, filesStd, propertyName);
        CLEAR_UNDO_STACK();
        break;
      }
    }
  }
}
