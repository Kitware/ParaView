/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputFileNameDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputFileNameDomain.h"
#include "vtkSMSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <vtksys/SystemTools.hxx>
#include <iostream>

vtkStandardNewMacro(vtkSMInputFileNameDomain);

//---------------------------------------------------------------------------
vtkSMInputFileNameDomain::vtkSMInputFileNameDomain()
{
  this->FileName = NULL;
}

//---------------------------------------------------------------------------
vtkSMInputFileNameDomain::~vtkSMInputFileNameDomain()
{
  this->FileName = 0;
}

//---------------------------------------------------------------------------
int vtkSMInputFileNameDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }
  vtkSMStringVectorProperty* sp = vtkSMStringVectorProperty::SafeDownCast(property);
  if (sp)
    {
    return 1;
    }

  return 0;
}


//---------------------------------------------------------------------------
void vtkSMInputFileNameDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{

  vtkSMProperty* propForInput = this->GetRequiredProperty("Input");
  if (!propForInput)
    {
    return;
    }

  vtkSMUncheckedPropertyHelper inputHelper(propForInput);

  if (inputHelper.GetAsProxy() != NULL)
    {
    vtkSMProperty * fileNameProperty = (inputHelper.GetAsProxy())->GetProperty(vtkSMCoreUtilities::GetFileNameProperty(inputHelper.GetAsProxy()));
    if (fileNameProperty != NULL)
      {
      std::string fname =  vtksys::SystemTools::DuplicateString(vtkSMPropertyHelper(fileNameProperty).GetAsString());
      fname = vtksys::SystemTools::GetFilenameName(fname);
      this->FileName = vtksys::SystemTools::DuplicateString(fname.c_str());
      this->DomainModified();
      return;
      }
    }
}


//---------------------------------------------------------------------------
void vtkSMInputFileNameDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
