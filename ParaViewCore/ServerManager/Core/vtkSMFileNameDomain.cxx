/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileNameDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFileNameDomain.h"
#include "vtkSMSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <vtksys/SystemTools.hxx>
#include <iostream>

vtkStandardNewMacro(vtkSMFileNameDomain);

//---------------------------------------------------------------------------
vtkSMFileNameDomain::vtkSMFileNameDomain()
{
  this->FileName = NULL;
}

//---------------------------------------------------------------------------
vtkSMFileNameDomain::~vtkSMFileNameDomain()
{
  this->FileName = 0;
}

//---------------------------------------------------------------------------
int vtkSMFileNameDomain::IsInDomain(vtkSMProperty* property)
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
void vtkSMFileNameDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{

  vtkSMProperty* propForInput = this->GetRequiredProperty("Input");
  if (!propForInput)
  {
    return;
  }

  vtkSMUncheckedPropertyHelper inputHelper(propForInput);
  vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  if (proxy != NULL)
  {
    vtkSmartPointer<vtkSMStringVectorProperty> fileNameProperty = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("FileName"));
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
void vtkSMFileNameDomain::SetAnimationValue(vtkSMProperty *prop, int idx,
                                              double vtkNotUsed(value))
{
  if (!prop)
    {
    return;
    }

  vtkSMStringVectorProperty *svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp)
    {
    svp->SetElement(idx, this->FileName);
    }
}

//---------------------------------------------------------------------------
void vtkSMFileNameDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
