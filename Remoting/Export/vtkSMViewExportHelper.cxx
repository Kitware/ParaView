/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewExportHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewExportHelper.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMDocumentation.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <sstream>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

vtkObjectFactoryNewMacro(vtkSMViewExportHelper);
//----------------------------------------------------------------------------
vtkSMViewExportHelper::vtkSMViewExportHelper() = default;

//----------------------------------------------------------------------------
vtkSMViewExportHelper::~vtkSMViewExportHelper() = default;

//----------------------------------------------------------------------------
std::string vtkSMViewExportHelper::GetSupportedFileTypes(vtkSMViewProxy* view)
{
  if (!view)
  {
    return std::string();
  }

  std::ostringstream stream;
  int count = 0;

  vtkSMSessionProxyManager* pxm = view->GetSessionProxyManager();
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("exporters"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkSMExporterProxy* prototype =
      vtkSMExporterProxy::SafeDownCast(pxm->GetPrototypeProxy("exporters", iter->GetProxyName()));
    if (prototype && prototype->CanExport(view) && !prototype->GetFileExtensions().empty())
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
        helpstream << vtksys::SystemTools::UpperCase(fileExtensionsStream.str().c_str())
                   << " Files";
      }
      stream << (count > 0 ? ";;" : "") << helpstream.str() << " (" << fileExtensionsStream.str()
             << ")";
      count++;
    }
  }
  return stream.str();
}

//----------------------------------------------------------------------------
vtkSMExporterProxy* vtkSMViewExportHelper::CreateExporter(
  const char* filename, vtkSMViewProxy* view)
{
  if (!view || filename == nullptr || filename[0] == '\0')
  {
    vtkErrorMacro("Invalid input arguments to Export.");
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = view->GetSessionProxyManager();
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("exporters"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkSMExporterProxy* prototype =
      vtkSMExporterProxy::SafeDownCast(pxm->GetPrototypeProxy("exporters", iter->GetProxyName()));
    if (prototype && prototype->CanExport(view))
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
          vtkSMExporterProxy* exporter =
            vtkSMExporterProxy::SafeDownCast(pxm->NewProxy("exporters", iter->GetProxyName()));
          vtkNew<vtkSMParaViewPipelineController> controller;
          controller->PreInitializeProxy(exporter);
          exporter->SetView(view);
          vtkSMPropertyHelper(exporter, "FileName").Set(filename);
          controller->PostInitializeProxy(exporter);
          exporter->UpdateVTKObjects();
          return exporter;
        }
      }
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMViewExportHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
