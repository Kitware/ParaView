// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMImporterFactory.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMDocumentation.h"
#include "vtkSMImporterProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include <sstream>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
std::string vtkSMImporterFactory::GetSupportedFileTypes(vtkSMSession* session)
{
  if (!session)
  {
    return {};
  }

  std::ostringstream stream;
  int count = 0;

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("importers"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkSMImporterProxy* prototype =
      vtkSMImporterProxy::SafeDownCast(pxm->GetPrototypeProxy("importers", iter->GetProxyName()));
    if (prototype && !prototype->GetFileExtensions().empty())
    {
      vtkSMDocumentation* doc = prototype->GetDocumentation();
      std::ostringstream helpstream;
      std::ostringstream fileExtensionsStream;
      std::vector<std::string> fileExtensions = prototype->GetFileExtensions();
      for (size_t i = 0; i < fileExtensions.size(); i++)
      {
        fileExtensionsStream << "*." << fileExtensions[i];
        if (i + 1 != fileExtensions.size())
        { // don't add in the space on the last entry to make it look better
          fileExtensionsStream << " ";
        }
      }
      if (doc && doc->GetShortHelp())
      {
        helpstream << doc->GetShortHelp();
      }
      else
      {
        helpstream << vtksys::SystemTools::UpperCase(fileExtensionsStream.str()) << " Files";
      }
      stream << (count > 0 ? ";;" : "") << helpstream.str() << " (" << fileExtensionsStream.str()
             << ")";
      count++;
    }
  }
  return stream.str();
}

//----------------------------------------------------------------------------
vtkSMImporterProxy* vtkSMImporterFactory::CreateImporter(
  const char* filename, vtkSMSession* session)
{
  if (filename == nullptr || filename[0] == '\0')
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("importers"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkSMImporterProxy* prototype =
      vtkSMImporterProxy::SafeDownCast(pxm->GetPrototypeProxy("importers", iter->GetProxyName()));
    if (prototype)
    {
      for (auto& ext : prototype->GetFileExtensions())
      {
        std::ostringstream reStream;
        reStream << "^"         // start
                 << ".*"        // leading text
                 << "\\."       // extension separator
                 << ext << "$"; // end
        vtksys::RegularExpression re(reStream.str().c_str());
        if (re.find(filename))
        {
          vtkSMImporterProxy* importer =
            vtkSMImporterProxy::SafeDownCast(pxm->NewProxy("importers", iter->GetProxyName()));
          vtkNew<vtkSMParaViewPipelineController> controller;
          controller->PreInitializeProxy(importer);
          importer->SetSession(session);
          vtkSMPropertyHelper(importer, "FileName").Set(filename);
          controller->PostInitializeProxy(importer);
          pxm->RegisterProxy("importers", importer);
          return importer;
        }
      }
    }
  }
  return nullptr;
}
