/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyStatusManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyStatusManager.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/map>
vtkStandardNewMacro(vtkSMPropertyStatusManager);
vtkCxxRevisionMacro(vtkSMPropertyStatusManager, "1.3");

//*****************************************************************************
class vtkSMPropertyStatusManagerInternals
{
public:
  typedef vtkstd::map<vtkSmartPointer<vtkSMVectorProperty>,
          vtkSmartPointer<vtkSMVectorProperty> > PropertyToPropertyMap;
  PropertyToPropertyMap Properties;
};

//*****************************************************************************
//-----------------------------------------------------------------------------

vtkSMPropertyStatusManager::vtkSMPropertyStatusManager()
{
  this->Internals = new vtkSMPropertyStatusManagerInternals;
}

//-----------------------------------------------------------------------------
vtkSMPropertyStatusManager::~vtkSMPropertyStatusManager()
{
  this->UnregisterAllProperties();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyStatusManager::RegisterProperty(vtkSMVectorProperty* property)
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.find(property);
  if (iter != this->Internals->Properties.end())
    {
    vtkErrorMacro("Property cannot be registered twice.");
    return;
    }
  vtkSMVectorProperty* newProp = this->DuplicateProperty(property);
  if (newProp == NULL)
    {
    vtkErrorMacro("Failed to register property.");
    return;
    }
  this->Internals->Properties[property] = newProp;
  newProp->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMPropertyStatusManager::UnregisterAllProperties()
{
  this->Internals->Properties.clear();
}

//-----------------------------------------------------------------------------
void vtkSMPropertyStatusManager::UnregisterProperty(vtkSMVectorProperty* property)
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.find(property);
  if (iter == this->Internals->Properties.end())
    {
    vtkErrorMacro("Property must be registered before unregistering.");
    return;
    }
  this->Internals->Properties.erase(iter);
}

//-----------------------------------------------------------------------------
void vtkSMPropertyStatusManager::InitializeStatus()
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.begin();
  for ( ; iter != this->Internals->Properties.end(); iter++)
    {
    this->DuplicateProperty(iter->first, iter->second);
    }
}

//-----------------------------------------------------------------------------
vtkSMVectorProperty* vtkSMPropertyStatusManager::GetInternalProperty(
  vtkSMVectorProperty* property)
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.find(property);
  if (iter == this->Internals->Properties.end())
    {
    vtkErrorMacro("Property is not registered with this property status manager.");
    return 0;
    }
  return iter->second;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyStatusManager::HasPropertyChanged(vtkSMVectorProperty* property)
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.find(property);
  if (iter == this->Internals->Properties.end())
    {
    vtkErrorMacro("Property is not registered with this property status manager.");
    return 0;
    }
  return HasPropertyChangedInternal(iter->first, iter->second, -1);
}

//-----------------------------------------------------------------------------
int vtkSMPropertyStatusManager::HasPropertyChanged(
  vtkSMVectorProperty* property, int index)
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.find(property);
  if (iter == this->Internals->Properties.end())
    {
    vtkErrorMacro("Property is not registered with this property status manager.");
    return 0;
    }
  return HasPropertyChangedInternal(iter->first, iter->second, index);
}

//-----------------------------------------------------------------------------
void vtkSMPropertyStatusManager::InitializePropertyStatus(vtkSMVectorProperty* property)
{
  vtkSMPropertyStatusManagerInternals::PropertyToPropertyMap::iterator iter =
    this->Internals->Properties.find(property);
  if (iter == this->Internals->Properties.end())
    {
    vtkErrorMacro("Property is not registered with this property status manager.");
    return;
    }
  this->DuplicateProperty(iter->first, iter->second);
}

//-----------------------------------------------------------------------------
int vtkSMPropertyStatusManager::HasPropertyChangedInternal(
  vtkSMVectorProperty* src, vtkSMVectorProperty* dest, int index)
{
  unsigned int cc;
  unsigned int num_elems = src->GetNumberOfElements();

  if (dest->GetNumberOfElements() != num_elems)
    {
    return 1;
    }

  if (static_cast<unsigned int>(index) >= num_elems)
    {// index is beyond range, we know nothing about it.
    return 0;
    }
  
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(src);
  vtkSMIntVectorProperty* sivp = vtkSMIntVectorProperty::SafeDownCast(src);
  vtkSMIdTypeVectorProperty* sidvp = vtkSMIdTypeVectorProperty::SafeDownCast(src);
  vtkSMStringVectorProperty* ssvp = vtkSMStringVectorProperty::SafeDownCast(src);

  vtkSMDoubleVectorProperty* ddvp = vtkSMDoubleVectorProperty::SafeDownCast(dest);
  vtkSMIntVectorProperty* divp = vtkSMIntVectorProperty::SafeDownCast(dest);
  vtkSMIdTypeVectorProperty* didvp = vtkSMIdTypeVectorProperty::SafeDownCast(dest);
  vtkSMStringVectorProperty* dsvp = vtkSMStringVectorProperty::SafeDownCast(dest);
  
  if (sdvp && ddvp)
    {
    if (index != -1)
      {
      return (ddvp->GetElement(index) != sdvp->GetElement(index));
      }
    for (cc=0; cc < num_elems; cc++)
      {
      if (ddvp->GetElement(cc) != sdvp->GetElement(cc))
        {
        return 1;
        }
      }
    return 0;
    }
  
  if (sivp && divp)
    {
    if (index != -1)
      {
      return (divp->GetElement(index) != sivp->GetElement(index));
      }
    for (cc=0; cc < num_elems; cc++)
      {
      if (divp->GetElement(cc) != sivp->GetElement(cc))
        {
        return 1;
        }
      }
    return 0;
    }

  if (sidvp && didvp)
    {
    if (index != -1)
      {
      return (didvp->GetElement(index) != sidvp->GetElement(index));
      }
    for (cc=0; cc < num_elems; cc++)
      {
      if (didvp->GetElement(cc) != sidvp->GetElement(cc))
        {
        return 1;
        }
      }
    return 0;
    }
  if (ssvp && dsvp)
    {
    if (index != -1)
      {
      return (strcmp(dsvp->GetElement(index),ssvp->GetElement(index))!=0);
      }
    for (cc=0; cc < num_elems; cc++)
      {
      if (strcmp(dsvp->GetElement(cc),ssvp->GetElement(cc))==0)
        {
        return 1;
        }
      }
    return 0;
    }
  vtkErrorMacro("Property type mismatch. Status not accurate.");
  return 0;
}

//-----------------------------------------------------------------------------
vtkSMVectorProperty* vtkSMPropertyStatusManager::DuplicateProperty(
  vtkSMVectorProperty* src, vtkSMVectorProperty* dest /*= NULL*/)
{
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(src);
  vtkSMIntVectorProperty* sivp = vtkSMIntVectorProperty::SafeDownCast(src);
  vtkSMIdTypeVectorProperty* sidvp = vtkSMIdTypeVectorProperty::SafeDownCast(src);
  vtkSMStringVectorProperty* ssvp = vtkSMStringVectorProperty::SafeDownCast(src);

  if (sdvp)
    {
    dest = (!dest)? vtkSMDoubleVectorProperty::New() : dest;
    dest->SetNumberOfElements(sdvp->GetNumberOfElements());
    vtkSMDoubleVectorProperty::SafeDownCast(dest)->SetElements(
      sdvp->GetElements());
    }
  else if (sivp)
    {
    dest = (!dest)? vtkSMIntVectorProperty::New() : dest;
    dest->SetNumberOfElements(sivp->GetNumberOfElements());
    vtkSMIntVectorProperty::SafeDownCast(dest)->SetElements(
      sivp->GetElements());
    }
  else if (sidvp)
    {
    dest = (!dest)? vtkSMIdTypeVectorProperty::New() : dest;
    unsigned int num_elems = sidvp->GetNumberOfElements();
    dest->SetNumberOfElements(num_elems);
    for(unsigned int cc=0; cc < num_elems; cc++)
      {
    vtkSMIdTypeVectorProperty::SafeDownCast(dest)->SetElement(cc,
      sidvp->GetElement(cc));
      }
    }
  else if (ssvp)
    {
    dest = (!dest)? vtkSMStringVectorProperty::New() : dest;
    unsigned int num_elems = ssvp->GetNumberOfElements();
    dest->SetNumberOfElements(num_elems);
    for (unsigned int cc=0; cc < num_elems; cc++)
      {
      vtkSMStringVectorProperty::SafeDownCast(dest)->SetElement(cc,
        ssvp->GetElement(cc));
      }
    }
  return dest;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyStatusManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
