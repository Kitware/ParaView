/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringListDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMStringListDomain);
vtkCxxRevisionMacro(vtkSMStringListDomain, "1.9");

struct vtkSMStringListDomainInternals
{
  vtkstd::vector<vtkStdString> Strings;
};

//---------------------------------------------------------------------------
vtkSMStringListDomain::vtkSMStringListDomain()
{
  this->SLInternals = new vtkSMStringListDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMStringListDomain::~vtkSMStringListDomain()
{
  delete this->SLInternals;
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringListDomain::GetNumberOfStrings()
{
  return this->SLInternals->Strings.size();
}

//---------------------------------------------------------------------------
const char* vtkSMStringListDomain::GetString(unsigned int idx)
{
  return this->SLInternals->Strings[idx].c_str();
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringListDomain::AddString(const char* string)
{
  this->SLInternals->Strings.push_back(string);
  return this->SLInternals->Strings.size() - 1;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::RemoveString(const char* string)
{
  if (!string)
    {
    return;
    }
  vtkstd::vector<vtkStdString>::iterator iter =
    this->SLInternals->Strings.begin();
  for(; iter != this->SLInternals->Strings.end(); iter++)
    {
    if (strcmp(string, iter->c_str()) == 0)
      {
      this->SLInternals->Strings.erase(iter);
      break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::RemoveAllStrings()
{
  this->SLInternals->Strings.erase(
    this->SLInternals->Strings.begin(), this->SLInternals->Strings.end());
}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }
  vtkSMStringVectorProperty* sp = 
    vtkSMStringVectorProperty::SafeDownCast(property);
  if (sp)
    {
    unsigned int numElems = sp->GetNumberOfUncheckedElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      unsigned int idx;
      if (!this->IsInDomain(sp->GetUncheckedElement(i), idx))
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMStringListDomain::IsInDomain(const char* val, unsigned int& idx)
{
  unsigned int numStrings = this->GetNumberOfStrings();
  if (numStrings == 0)
    {
    return 1;
    }

  for (unsigned int i=0; i<numStrings; i++)
    {
    if (strcmp(val, this->GetString(i)) == 0)
      {
      idx = i;
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::Update(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && svp->GetInformationOnly())
    {
    this->RemoveAllStrings();
    unsigned int numStrings = svp->GetNumberOfElements();
    for (unsigned int i=0; i<numStrings; i++)
      {
      this->AddString(svp->GetElement(i));
      }
    }
}


//---------------------------------------------------------------------------
int vtkSMStringListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
    {
    return 0;
    }

  // Loop over the top-level elements.
  unsigned int i;
  for(i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* selement = element->GetNestedElement(i);
    if ( strcmp("String", selement->GetName()) != 0 )
      {
      continue;
      }
    const char* value = selement->GetAttribute("value");
    if (!value)
      {
      vtkErrorMacro(<< "Can not find required attribute: value. "
                    << "Can not parse domain xml.");
      return 0;
      }

    this->AddString(value);
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::SaveState(
  const char* name, ostream* file, vtkIndent indent)
{
  *file << indent 
        << "<Domain name=\"" << this->XMLName << "\" id=\"" << name << "\">"
        << endl;
  unsigned int size = this->GetNumberOfStrings();
  for(unsigned int i=0; i<size; i++)
    {
    *file << indent.GetNextIndent() 
          << "<String text=\"" << this->GetString(i)
          << "\"/>" << endl;
    }
      
  *file << indent
        << "</Domain>" << endl;
    
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::SetAnimationValue(vtkSMProperty *prop, int idx,
                                              double value)
{
  if (!prop)
    {
    return;
    }

  vtkSMStringVectorProperty *svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp)
    {
    svp->SetElement(idx, this->GetString((int)(floor(value))));
    }
}

//---------------------------------------------------------------------------
void vtkSMStringListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
