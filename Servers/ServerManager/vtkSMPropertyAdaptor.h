/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyAdaptor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropertyAdaptor - provides string based interface for properties and domains
// .SECTION Description
// vtkSMPropertyAdaptor provides a general purpose string based interface
// for properties and domain. This is a helper class that can be used
// to simplify the management of properties and domains although it
// somehow restricts the capabilities of the server manager.

#ifndef __vtkSMPropertyAdaptor_h
#define __vtkSMPropertyAdaptor_h

#include "vtkSMObject.h"

class vtkSMDomain;
class vtkSMBooleanDomain;
class vtkSMDoubleRangeDomain;
class vtkSMEnumerationDomain;
class vtkSMIntRangeDomain;
class vtkSMProxyGroupDomain;
class vtkSMStringListDomain;

class vtkSMProperty;
class vtkSMProxyProperty;
class vtkSMDoubleVectorProperty;
class vtkSMIdTypeVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMStringVectorProperty;

class VTK_EXPORT vtkSMPropertyAdaptor : public vtkSMObject
{
public:
  static vtkSMPropertyAdaptor* New();
  vtkTypeRevisionMacro(vtkSMPropertyAdaptor, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the property to be adapted. The property
  // has to be set before any other method can be called.
  void SetProperty(vtkSMProperty* prop);
  vtkGetObjectMacro(Property, vtkSMProperty);

  // Description:
  // If the property is of type RANGE, these return min
  // and max values as strings. If the property is of
  // another type or no min/max exists, 0 returned.
  const char* GetMinimum(unsigned int idx);
  const char* GetMaximum(unsigned int idx);

  // Description:
  // If the property is of type ENUMERATION, returns the
  // number of enumeration entries. Returns 0 otherwise
  unsigned int GetNumberOfEnumerationEntries();

  // Description:
  // If the property is of type ENUMERATION, returns an
  // enumeration value as a string. Returns 0 otherwise.
  const char* GetEnumerationValue(unsigned int idx);

  // Description:
  // If the property is of type ENUMERATION, returns a
  // current index of a value.
  unsigned int GetEnumerationElementIndex(const char* element);

  // Description:
  // Returns the number of property elements.
  unsigned int GetNumberOfElements();


  // Description:
  // Returns a property element.
  const char* GetElement(unsigned int idx);

  // Description:
  // Set a property element. Returns 1 on success, 0 on
  // failure.
  int SetElement(unsigned int idx, const char* value);

  // Description:
  // Returns either ENUMERATION, RANGE or UNKNOWN.
  int GetPropertyType();

//BTX
  enum PropertyTypes
  {
    UNKNOWN,
    ENUMERATION,
    RANGE
  };
//ETX

protected:
  vtkSMPropertyAdaptor();
  ~vtkSMPropertyAdaptor();

  void InitializeDomains();
  void InitializeProperties();

  void SetDomain(vtkSMDomain* domain);

  vtkSMBooleanDomain* BooleanDomain;
  vtkSMDoubleRangeDomain* DoubleRangeDomain;
  vtkSMEnumerationDomain* EnumerationDomain;
  vtkSMIntRangeDomain* IntRangeDomain;
  vtkSMProxyGroupDomain* ProxyGroupDomain;
  vtkSMStringListDomain* StringListDomain;

  vtkSMProxyProperty* ProxyProperty;
  vtkSMDoubleVectorProperty* DoubleVectorProperty;
  vtkSMIdTypeVectorProperty* IdTypeVectorProperty;
  vtkSMIntVectorProperty* IntVectorProperty;
  vtkSMStringVectorProperty* StringVectorProperty;

  vtkSMProperty* Property;

  char Minimum[128];
  char Maximum[128];
  char EnumValue[128];
  char ElemValue[128];

  virtual void SaveState(const char*, ostream*, vtkIndent) {}

private:
  vtkSMPropertyAdaptor(const vtkSMPropertyAdaptor&); // Not implemented
  void operator=(const vtkSMPropertyAdaptor&); // Not implemented
};

#endif
