/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCoreUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCoreUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMDomain.h"

vtkStandardNewMacro(vtkSMCoreUtilities);
//----------------------------------------------------------------------------
vtkSMCoreUtilities::vtkSMCoreUtilities()
{
}

//----------------------------------------------------------------------------
vtkSMCoreUtilities::~vtkSMCoreUtilities()
{
}

//----------------------------------------------------------------------------
const char* vtkSMCoreUtilities::GetFileNameProperty(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return NULL;
    }

  if (proxy->GetHints())
    {
    vtkPVXMLElement* filenameHint =
      proxy->GetHints()->FindNestedElementByName("DefaultFileNameProperty");
    if (filenameHint &&
      filenameHint->GetAttribute("name") &&
      proxy->GetProperty(filenameHint->GetAttribute("name")))
      {
      return filenameHint->GetAttribute("name");
      }
    }

  // Find the first property that has a vtkSMFileListDomain. Assume that
  // it is the property used to set the filename.
  vtkSmartPointer<vtkSMPropertyIterator> piter;
  piter.TakeReference(proxy->NewPropertyIterator());
  piter->Begin();
  while (!piter->IsAtEnd())
    {
    vtkSMProperty* prop = piter->GetProperty();
    if (prop && prop->IsA("vtkSMStringVectorProperty"))
      {
      vtkSmartPointer<vtkSMDomainIterator> diter;
      diter.TakeReference(prop->NewDomainIterator());
      diter->Begin();
      while(!diter->IsAtEnd())
        {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
          {
          return piter->GetKey();
          }
        diter->Next();
        }
      if (!diter->IsAtEnd())
        {
        break;
        }
      }
    piter->Next();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMCoreUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
