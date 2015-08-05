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
int vtkSMInputFileNameDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp)
    {
    vtkSMPropertyHelper helper(prop);
    helper.SetUseUnchecked(use_unchecked_values);
    const char* defaultValue = svp->GetDefaultValue(0);
    unsigned int temp;
    if (defaultValue && this->IsInDomain(defaultValue, temp))
      {
      helper.Set(0, defaultValue);
      }
    else
      {
      helper.Set(0, "(No File Name)");
      }
    return 1;  
    }
  return 0;
}


//---------------------------------------------------------------------------
void vtkSMInputFileNameDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
