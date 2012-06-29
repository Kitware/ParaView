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
// It supports three types of properties:
// @verbatim
// 1. Vector properties with range (RANGE)
// 2. Enumaration properties (ENUMERATION or FILE_LIST)
// 3. Selection properties (SELECTION)
// @endverbatim
// (1) represent int or double properties that are restricted between a min
// and a max. (2) represent properties that can have a value out of a list
// (for example, the representation of a geometry: solid, wireframe, points
// etc.. (3) represent properties that associate a value with a key
// (similar to maps). This might be used to set the active state of arrays
// for example. Make sure to use the API appropriate for the property
// type. Using the wrong API will produce incorrect results.

#ifndef __vtkSMPropertyAdaptor_h
#define __vtkSMPropertyAdaptor_h

#include "vtkSMObject.h"

class vtkSMDomain;
class vtkSMBooleanDomain;
class vtkSMDoubleRangeDomain;
class vtkSMEnumerationDomain;
class vtkSMFileListDomain;
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
  vtkTypeMacro(vtkSMPropertyAdaptor, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the property to be adapted. The property
  // has to be set before any other method can be called.
  void SetProperty(vtkSMProperty* prop);
  vtkGetObjectMacro(Property, vtkSMProperty);

  // ----------------------------------------------------
  // Vector properties (int and double) with range

  // Description:
  // Return min and max as string. Returns NULL if min or
  // max is not set.
  const char* GetRangeMinimum(unsigned int idx);
  const char* GetRangeMaximum(unsigned int idx);

  // Description:
  // Returns the number of elements.
  unsigned int GetNumberOfRangeElements();

  // Description:
  // Returns the property value.
  const char* GetRangeValue(unsigned int idx);

  // Description:
  // Sets the property value.
  int SetRangeValue(unsigned int idx, const char* value);

  // ----------------------------------------------------
  // Enumeration properties

  // Description:
  // Returns the number possible enumeration entries. Note that
  // this is different than the number of elements in the property.
  unsigned int GetNumberOfEnumerationElements();

  // Description:
  // Returns the string associated with an enumeration entry.
  const char* GetEnumerationName(unsigned int idx);

  // Description:
  // Returns the value. Note that this is an int (converted to
  // string) that should be used together with GetEnumerationName.
  const char* GetEnumerationValue();

  // Description:
  // Set the value. Should be an int.
  int SetEnumerationValue(const char* idx);

  // ----------------------------------------------------
  // Selection properties

  // Description:
  // Returns the number of elements that can be set/get.
  unsigned int GetNumberOfSelectionElements();

  // Description:
  // Returns a string representation for the name of
  // a selection element.
  const char* GetSelectionName(unsigned int idx);

  // Description:
  // Returns the value of an element.
  const char* GetSelectionValue(unsigned int idx);

  // Description:
  // Set the value.
  int SetSelectionValue(unsigned int idx, const char* value);

  // Description:
  // Returns the min and max for a selection element. Returns
  // NULL if min or max are not set.
  const char* GetSelectionMinimum(unsigned int idx);
  const char* GetSelectionMaximum(unsigned int idx);

  // ----------------------------------------------------
  // General

  // Description:
  // Initialize the underlying property from it's information
  // property by copying the values of the information property
  // to the propery.
  void InitializePropertyFromInformation();

  // Description:
  // Returns either ENUMERATION, RANGE, SELECTION, FILE_LIST or UNKNOWN.
  int GetPropertyType();

  // Description:
  // Returns either INT, DOUBLE, STRING, PROXY, BOOLEAN or UNKNOWN
  int GetElementType();

  // Description:
  // Set the value of the property. Use this if none of the three
  // Set methods above are not appropriate.
  int SetGenericValue(unsigned int idx, const char* value);

//BTX
  enum PropertyTypes
  {
    UNKNOWN = 0,
    ENUMERATION,
    SELECTION,
    RANGE,
    FILE_LIST,
    NUM_PROPERTY_TYPES
  };

  enum ElementType
  {
    INT = NUM_PROPERTY_TYPES+1,
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
  vtkSMFileListDomain* FileListDomain;
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

private:
  vtkSMPropertyAdaptor(const vtkSMPropertyAdaptor&); // Not implemented
  void operator=(const vtkSMPropertyAdaptor&); // Not implemented
};

#endif
