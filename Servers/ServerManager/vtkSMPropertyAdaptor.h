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
class vtkSMStringListRangeDomain;

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

  // ----------------------------------------------------
  // Vector properties (int and double) with range

  // Description:
  const char* GetRangeMinimum(unsigned int idx);
  const char* GetRangeMaximum(unsigned int idx);

  // Description:
  unsigned int GetNumberOfRangeElements();

  // Description:
  const char* GetRangeValue(unsigned int idx);

  // Description:
  int SetRangeValue(unsigned int idx, const char* value);

  // ----------------------------------------------------
  // Enumeration properties

  // Description:
  unsigned int GetNumberOfEnumerationElements();

  // Description:
  const char* GetEnumerationName(unsigned int idx);

  // Description:
  const char* GetEnumerationValue();

  // Description:
  int SetEnumerationValue(const char* idx);

  // ----------------------------------------------------
  // Selection properties

  // Description:
  unsigned int GetNumberOfSelectionElements();

  // Description:
  const char* GetSelectionName(unsigned int idx);

  // Description:
  const char* GetSelectionValue(unsigned int idx);

  // Description:
  int SetSelectionValue(unsigned int idx, const char* value);

  // Description:
  const char* GetSelectionMinimum(unsigned int idx);
  const char* GetSelectionMaximum(unsigned int idx);

  // ----------------------------------------------------
  // General

  // Description:
  void InitializePropertyFromInformation();

  // Description:
  // Returns either ENUMERATION, RANGE, SELECTION or UNKNOWN.
  int GetPropertyType();

  // Description:
  // Returns either INT, DOUBLE, STRING, PROXY, BOOLEAN or UNKNOWN
  int GetElementType();

  // Description:
  int SetGenericValue(unsigned int idx, const char* value);

//BTX
  enum PropertyTypes
  {
    UNKNOWN = 0,
    ENUMERATION,
    SELECTION,
    RANGE
  };

  enum ElementType
  {
    INT = RANGE + 1,
    DOUBLE,
    STRING,
    BOOLEAN,
    PROXY
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
  vtkSMStringListRangeDomain* StringListRangeDomain;

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
