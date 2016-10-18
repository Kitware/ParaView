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

vtkCxxSetObjectMacro(vtkSMExporterProxy, View, vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkSMExporterProxy::vtkSMExporterProxy()
{
  this->View = 0;
  this->FileExtension = 0;
  this->SetFileExtension("txt");
  this->SetLocation(vtkProcessModule::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMExporterProxy::~vtkSMExporterProxy()
{
  this->SetView(0);
  this->SetFileExtension(0);
}

//----------------------------------------------------------------------------
int vtkSMExporterProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pxm, vtkPVXMLElement* element)
{
  const char* exts = element->GetAttribute("file_extension");
  if (exts)
  {
    this->SetFileExtension(exts);
  }

  return this->Superclass::ReadXMLAttributes(pxm, element);
}

//----------------------------------------------------------------------------
void vtkSMExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "View: " << this->View << endl;
  os << indent << "FileExtension: " << (this->FileExtension ? this->FileExtension : "(none)")
     << endl;
}
