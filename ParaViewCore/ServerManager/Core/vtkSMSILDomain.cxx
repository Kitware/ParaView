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
#include "vtkPVSILInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMSILDomain);
//----------------------------------------------------------------------------
vtkSMSILDomain::vtkSMSILDomain()
{
  this->SubTree = 0;
  this->SILTimeStamp = 0;
  this->SIL = vtkPVSILInformation::New();
}

//----------------------------------------------------------------------------
vtkSMSILDomain::~vtkSMSILDomain()
{
  this->SetSubTree(0);
  this->SIL->Delete();
}

//----------------------------------------------------------------------------
int vtkSMSILDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem)
{
  if (!this->Superclass::ReadXMLAttributes(prop, elem))
  {
    return 0;
  }

  // Keep subtree attribute
  this->SetSubTree(elem->GetAttribute("subtree"));
  return 1;
}
//----------------------------------------------------------------------------
vtkGraph* vtkSMSILDomain::GetSIL()
{
  vtkSMIdTypeVectorProperty* timestamp =
    vtkSMIdTypeVectorProperty::SafeDownCast(this->GetRequiredProperty("TimeStamp"));
  vtkSMProperty* silProp = this->GetRequiredProperty("ArrayList");

  if (timestamp != NULL)
  {
    // Check timestamp to know if the SIL fetch is needed
    timestamp->GetParent()->UpdatePropertyInformation(timestamp);

    if (timestamp->GetNumberOfElements() == 0)
    {
      timestamp->GetParent()->GatherInformation(this->SIL);
    }
    else if (this->SILTimeStamp < timestamp->GetElement(0))
    {
      this->SILTimeStamp = timestamp->GetElement(0);
      timestamp->GetParent()->GatherInformation(this->SIL);
    }
  }
  else if (silProp != NULL)
  {
    // With no timestamp, we simply fecth each time the request is made
    silProp->GetParent()->GatherInformation(this->SIL);
  }

  return this->SIL->GetSIL();
}

//----------------------------------------------------------------------------
void vtkSMSILDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
