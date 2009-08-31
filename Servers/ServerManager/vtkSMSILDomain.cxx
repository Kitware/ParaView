/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSILDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSILDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMSILInformationHelper.h"

vtkStandardNewMacro(vtkSMSILDomain);
vtkCxxRevisionMacro(vtkSMSILDomain, "1.2");
//----------------------------------------------------------------------------
vtkSMSILDomain::vtkSMSILDomain()
{
}

//----------------------------------------------------------------------------
vtkSMSILDomain::~vtkSMSILDomain()
{
}

//----------------------------------------------------------------------------
const char* vtkSMSILDomain::GetSubtree()
{
  vtkSMProperty* req_prop = this->GetRequiredProperty("ArrayList");
  if (!req_prop)
    {
    vtkErrorMacro("Required property 'ArrayList' missing."
      "Cannot fetch the SIL");
    return 0;
    }

  vtkSMSILInformationHelper* helper =
    vtkSMSILInformationHelper::SafeDownCast(req_prop->GetInformationHelper());
  if (!helper)
    {
    vtkErrorMacro("Failed to locate vtkSMSILInformationHelper.");
    return 0;
    }

  return helper->GetSubtree();
}


//----------------------------------------------------------------------------
vtkGraph* vtkSMSILDomain::GetSIL()
{
  vtkSMProperty* req_prop = this->GetRequiredProperty("ArrayList");
  if (!req_prop)
    {
    vtkErrorMacro("Required property 'ArrayList' missing."
      "Cannot fetch the SIL");
    return 0;
    }

  vtkSMSILInformationHelper* helper =
    vtkSMSILInformationHelper::SafeDownCast(req_prop->GetInformationHelper());
  if (!helper)
    {
    vtkErrorMacro("Failed to locate vtkSMSILInformationHelper.");
    return 0;
    }

  return helper->GetSIL();
}

//----------------------------------------------------------------------------
void vtkSMSILDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


