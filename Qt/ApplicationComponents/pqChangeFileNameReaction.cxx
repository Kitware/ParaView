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

#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include <vtksys/SystemTools.hxx>

#include <QInputDialog>

#include <string>
#include <unordered_map>
#include <vector>

namespace
{
//-----------------------------------------------------------------------------
void CreateReader(vtkSMSourceProxy* proxy, const QStringList& files, const char* propName)
{
  auto newProxy = vtkSmartPointer<vtkSMSourceProxy>::Take(vtkSMSourceProxy::SafeDownCast(
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->NewProxy(
      proxy->GetXMLGroup(), proxy->GetXMLName())));
  auto fileNameProp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(propName));
  auto newFileNameProp = vtkSMStringVectorProperty::SafeDownCast(newProxy->GetProperty(propName));

  std::set<std::string> fileNames;
  for (unsigned int fileId = 0; fileId < fileNameProp->GetNumberOfElements(); ++fileId)
  {
    fileNames.insert(std::string(fileNameProp->GetElement(fileId)));
  }

  unsigned int numberOfCommonFiles = 0;
  for (const QString& file : files)
  {
    numberOfCommonFiles += static_cast<unsigned int>(fileNames.count(file.toStdString()));
  }

  if (numberOfCommonFiles == fileNameProp->GetNumberOfElements())
  {
    // No need to create a new reader, the user just selected the same set of files.
    return;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(newProxy);

  for (const QString& file : files)
  {
    newFileNameProp->SetElement(0, file.toStdString().c_str());
  }

  newProxy->UpdateVTKObjects();
  controller->PostInitializeProxy(newProxy);

  // We want to copy properties from the old proxy to the new proxy.  We have to handle array
  // names very carefully:
  // We create a lookup from array name to its state ("1" or "0" depending of if it is checked).
  // Then, we iterate on the new proxy, and for any array that was already present in the old proxy,
  // we copy its state.
  // This is only valid for properties that have a vtkSMArraySelectionDomain.
  // For other properties, we blindly copy what once was.
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> arraySelectionState;

  // Creating the lookup
  {
    auto iter = vtkSmartPointer<vtkSMPropertyIterator>::Take(proxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      if (auto prop = vtkSMStringVectorProperty::SafeDownCast(iter->GetProperty()))
      {
        // We only want to copy array selections, which shall have 2 elements per command
        // and have a pattern [str int] for each command.
        if (prop->FindDomain<vtkSMArraySelectionDomain>() &&
          prop->GetNumberOfElementsPerCommand() == 2 && prop->GetElementType(0) == 2 &&
          prop->GetElementType(1) == 0)
        {
          const std::vector<std::string>& elements = prop->GetElements();
          auto& selection = arraySelectionState[std::string(iter->GetKey())];
          for (int elementId = 0; elementId < static_cast<int>(elements.size()); elementId += 2)
          {
            // inserting elements of style { "name", "0" }  or { "name", "1" }
            selection[elements[elementId]] = elements[elementId + 1];
          }
        }
      }
    }
  }

  // Copying
  {
    auto iter = vtkSmartPointer<vtkSMPropertyIterator>::Take(newProxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      const char* key = iter->GetKey();
      vtkSMProperty* newProp = iter->GetProperty();
      if (newProp)
      {
        if (strcmp(key, propName) != 0)
        {
          if (vtkSMProperty* prop = proxy->GetProperty(key))
          {
            auto newPropStr = vtkSMStringVectorProperty::SafeDownCast(newProp);

            // For properties holding arrays, we copy the previous state as much as we can
            if (newPropStr && newPropStr->FindDomain<vtkSMArraySelectionDomain>() &&
              newPropStr->GetNumberOfElementsPerCommand() == 2 &&
              newPropStr->GetElementType(0) == 2 && newPropStr->GetElementType(1) == 0)
            {
              const std::vector<std::string>& elements = newPropStr->GetElements();
              const auto& selection = arraySelectionState.at(std::string(key));
              for (int elementId = 0; elementId < static_cast<int>(elements.size()); elementId += 2)
              {
                auto it = selection.find(elements[elementId]);
                if (it != selection.end())
                {
                  newPropStr->SetElement(elementId + 1, it->second.c_str());
                }
              }
            }
            // For other properties, we just copy
            else if (!vtkSMProxyProperty::SafeDownCast(newProp))
            {
              newProp->Copy(prop);
            }
          }
        }
      }
    }
  }
  newProxy->UpdateVTKObjects();

  QString fileName = files.size() > 1
    ? pqCoreUtilities::findLargestPrefix(files)
    : QString(vtksys::SystemTools::GetFilenameName(files[0].toStdString()).c_str());

  // We need to register the new proxy before we change the input property of the consumers of the
  // legacy proxy
  controller->RegisterPipelineProxy(newProxy, fileName.toStdString().c_str());

  // For some reason, consumers from proxy get wiped out when setting new input proxies,
  // so we rely on a copy of the consumers
  unsigned int numberOfConsumers = proxy->GetNumberOfConsumers();
  std::vector<vtkSmartPointer<vtkSMProxy>> consumers(numberOfConsumers);
  std::vector<vtkSmartPointer<vtkSMProperty>> consumerProperties(numberOfConsumers);
  for (unsigned int id = 0; id < numberOfConsumers; ++id)
  {
    consumers[id] = proxy->GetConsumerProxy(id);
    consumerProperties[id] = proxy->GetConsumerProperty(id);
  }

  // Setting up consumers input to new proxy
  for (unsigned int id = 0; id < numberOfConsumers; ++id)
  {
    vtkSMProxy* consumer = consumers[id];
    auto prop = vtkSMProxyProperty::SafeDownCast(consumerProperties[id]);
    if (!prop)
    {
      continue;
    }
    unsigned int proxyId = 0;
    for (; proxyId < prop->GetNumberOfProxies(); ++proxyId)
    {
      if (proxy == prop->GetProxy(proxyId))
      {
        break;
      }
    }
    vtkSMPropertyHelper(prop).Set(proxyId, newProxy);
    consumer->UpdateVTKObjects();
  }

  // We don't need the legacy proxy anymore, we can delete it.
  controller->UnRegisterPipelineProxy(proxy);

  newProxy->UpdatePipeline();
}
} // anonymous namespace

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

    if (vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("FileName")))
    {
      ::CreateReader(proxy, files, "FileName");
    }
    else if (vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("FileNames")))
    {
      ::CreateReader(proxy, files, "FileNames");
    }
    else
    {
      qWarning("FileName or FileNames property not present in the source, or is of wrong type.");
    }
  }
}
