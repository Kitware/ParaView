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

#include <sstream>
#include <ctype.h>
#include <cassert>
#include <cmath>

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

  std::ostringstream cname;
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
bool vtkSMCoreUtilities::AdjustRangeForLog(double range[2])
{
  assert(range[0] <= range[1]);
  if (range[0] <= 0.0 || range[1] <= 0.0)
    {
    // ranges not valid for log-space. Cannot convert.
    if (range[1] <= 0.0)
      {
      range[0] = 1.0e-4;
      range[1] = 1.0;
      }
    else
      {
      range[0] =  range[1] * 0.0001;
      range[0] =  (range[0] < 1.0)? range[0] : 1.0;
      }
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMCoreUtilities::AdjustRange(double range[2])
{
  if (range[1] < range[0])
    {
    // invalid range.
    return false;
    }

  // the range must be large enough, compared to values order of magnitude
  double rangeOrderOfMagnitude = 1e-11;
  if (rangeOrderOfMagnitude < std::abs(range[0]))
    {
    rangeOrderOfMagnitude = std::abs(range[0]);
    }
  if (rangeOrderOfMagnitude < std::abs(range[1]))
    {
    rangeOrderOfMagnitude = std::abs(range[1]);
    }
  double rangeMinLength = rangeOrderOfMagnitude * 1e-5;
  if ((range[1] - range[0]) < rangeMinLength)
    {
    range[1] = range[0] + rangeMinLength;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMCoreUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
