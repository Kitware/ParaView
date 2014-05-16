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

#include <vtksys/ios/sstream>
#include <ctype.h>

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
vtkStdString vtkSMCoreUtilities::SanitizeName(const char* name)
{
  if (!name || name[0] == '\0')
    {
    return vtkStdString();
    }

  vtksys_ios::ostringstream cname;
  for (size_t cc=0; name[cc]; cc++)
    {
    if (isalnum(name[cc]))
      {
      cname << name[cc];
      }
    }
  // if first character is not an alphabet, add an 'a' to it.
  if (isalpha(cname.str()[0]))
    {
    return cname.str();
    }
  else
    {
    return "a" + cname.str();
    }
}

//----------------------------------------------------------------------------
void vtkSMCoreUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
