/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVPluginInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPluginProxy);

//----------------------------------------------------------------------------
vtkSMPluginProxy::vtkSMPluginProxy()
{
  this->PluginInfo = vtkPVPluginInformation::New(); 
}

//----------------------------------------------------------------------------
vtkSMPluginProxy::~vtkSMPluginProxy()
{
  this->PluginInfo->Delete();
  this->PluginInfo = 0;
}

//----------------------------------------------------------------------------
vtkPVPluginInformation* vtkSMPluginProxy::Load(const char* filename)
{
  if(!this->GetProperty("Loaded"))
    {
    vtkErrorMacro("The plugin proxy don't have Loaded property!");
    return 0;
    }

  // Set Plugin filename
  vtkSMStringVectorProperty* filenameProperty;
  filenameProperty = vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("FileName"));
  filenameProperty->SetElement(0, filename);

  // Make the network communication
  this->UpdateVTKObjects();
  this->UpdatePropertyInformation();
  this->GatherInformation(this->PluginInfo);

  return this->PluginInfo;
}

//----------------------------------------------------------------------------
void vtkSMPluginProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PluginInfo: "  << endl;
  this->PluginInfo->PrintSelf(os, indent.GetNextIndent());
}
