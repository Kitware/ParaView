/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExporterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExporterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMViewProxy.h"

#include <vtksys/SystemTools.hxx>

vtkCxxSetObjectMacro(vtkSMExporterProxy, View, vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkSMExporterProxy::vtkSMExporterProxy()
{
  this->View = nullptr;
  this->FileExtensions.push_back("txt");
  this->SetLocation(vtkProcessModule::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMExporterProxy::~vtkSMExporterProxy()
{
  this->SetView(nullptr);
}

//----------------------------------------------------------------------------
int vtkSMExporterProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pxm, vtkPVXMLElement* element)
{
  // we let the superclass read in information first so that we can just
  // get the proper hints (i.e. not hints from base proxies) after that to
  // figure out file extensions
  int retVal = this->Superclass::ReadXMLAttributes(pxm, element);

  bool addedExtension = false;
  if (const char* exts = element->GetAttribute("file_extension"))
  {
    vtkWarningMacro("Export proxy definition for file_extension has been deprecated."
      << " Use ExporterFactory extensions in Hints instead.");
    this->FileExtensions[0] = exts;
    addedExtension = true;
  }
  if (vtkPVXMLElement* hintElement = this->GetHints())
  {
    vtkPVXMLElement* exporterFactoryElement =
      hintElement->FindNestedElementByName("ExporterFactory");
    if (exporterFactoryElement)
    {
      if (const char* e = exporterFactoryElement->GetAttribute("extensions"))
      {
        std::string extensions = e;
        std::vector<std::string> extensionsVec;
        vtksys::SystemTools::Split(extensions, extensionsVec, ' ');
        for (auto iter = extensionsVec.begin(); iter != extensionsVec.end(); iter++)
        {
          if (addedExtension)
          {
            this->FileExtensions.push_back(*iter);
          }
          else
          {
            this->FileExtensions[0] = *iter;
            addedExtension = true;
          }
        }
      }
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
void vtkSMExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "View: " << this->View << endl;
  os << indent << "FileExtensions:";
  for (const auto& fname : this->FileExtensions)
  {
    os << " " << fname;
  }
  os << endl;
}
