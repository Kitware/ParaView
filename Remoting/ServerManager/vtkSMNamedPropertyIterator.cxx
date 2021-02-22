/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedPropertyIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNamedPropertyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyInternals.h"
#include "vtkStringList.h"

vtkStandardNewMacro(vtkSMNamedPropertyIterator);

typedef vtkSMProxyInternals::PropertyInfoMap::iterator PropertyIterator;
typedef vtkSMProxyInternals::ExposedPropertyInfoMap::iterator ExposedPropertyIterator;

//---------------------------------------------------------------------------
vtkSMNamedPropertyIterator::vtkSMNamedPropertyIterator()
  : PropertyNames(nullptr)
  , PropertyNameIndex(0)
{
}

//---------------------------------------------------------------------------
vtkSMNamedPropertyIterator::~vtkSMNamedPropertyIterator()
{
  this->SetPropertyNames(nullptr);
}

//---------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkSMNamedPropertyIterator, PropertyNames, vtkStringList);

//---------------------------------------------------------------------------
void vtkSMNamedPropertyIterator::Begin()
{
  this->PropertyNameIndex = 0;
}

//---------------------------------------------------------------------------
int vtkSMNamedPropertyIterator::IsAtEnd()
{
  if (!this->PropertyNames)
  {
    vtkErrorMacro("PropertyNames is not set. Can not perform operation: IsAtEnd()");
    return 0;
  }

  return this->PropertyNameIndex >= this->PropertyNames->GetNumberOfStrings();
}

//---------------------------------------------------------------------------
void vtkSMNamedPropertyIterator::Next()
{
  ++this->PropertyNameIndex;
}

//---------------------------------------------------------------------------
const char* vtkSMNamedPropertyIterator::GetKey()
{
  if (!this->PropertyNames)
  {
    vtkErrorMacro("PropertyNames is not set. Can not perform operation: GetKey()");
    return nullptr;
  }

  return this->PropertyNames->GetString(this->PropertyNameIndex);
}

//---------------------------------------------------------------------------
const char* vtkSMNamedPropertyIterator::GetPropertyLabel()
{
  return this->GetProperty()->GetXMLLabel();
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMNamedPropertyIterator::GetProperty()
{
  if (!this->PropertyNames)
  {
    vtkErrorMacro("PropertyNames is not set. Can not perform operation: GetProperty()");
    return nullptr;
  }
  if (!this->Proxy)
  {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetProperty()");
    return nullptr;
  }

  // get the requested prooperty's name, it's the key into
  // the map.
  std::string name = this->PropertyNames->GetString(this->PropertyNameIndex);

  // Is the requested property in this proxy?
  PropertyIterator propEnd = this->Proxy->Internals->Properties.end();
  PropertyIterator propIt = this->Proxy->Internals->Properties.find(name);

  // Yes, we have it.
  if (propIt != propEnd)
  {
    return propIt->second.Property.GetPointer();
  }

  // No, but it may be in the exposed properties.
  if (this->TraverseSubProxies)
  {
    ExposedPropertyIterator expPropEnd = this->Proxy->Internals->ExposedProperties.end();
    ExposedPropertyIterator expPropIt = this->Proxy->Internals->ExposedProperties.find(name);

    // Yes, we have it.
    if (expPropIt != expPropEnd)
    {
      const char* subProxyName = expPropIt->second.SubProxyName.c_str();
      vtkSMProxy* subProxy = this->Proxy->GetSubProxy(subProxyName);
      // The sub proxy should always be present.
      if (!subProxy)
      {
        vtkErrorMacro(<< "In proxy " << this->Proxy->GetXMLName() << " cannot find sub proxy "
                      << subProxyName << ".");
        return nullptr;
      }
      const char* expPropName = expPropIt->second.PropertyName.c_str();
      vtkSMProperty* expProp = subProxy->GetProperty(expPropName);
      // the property should always be present.
      if (!expProp)
      {
        vtkErrorMacro(<< "In proxy " << this->Proxy->GetXMLName()
                      << " cannot find exposed property " << name.c_str() << "."
                      << " Which is expected to be " << expPropName << " of " << subProxyName
                      << ".");
      }
      return expProp;
    }
  }

  // Exhausted all the possibilities, we do not have the requested
  // property.
  vtkErrorMacro(<< "In proxy " << this->Proxy->GetXMLName() << " no property named " << name.c_str()
                << " was found.");
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMNamedPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PropertyNames: " << this->PropertyNames << endl;
  os << indent << "PropertyNameIndex: " << this->PropertyNameIndex << endl;
}
