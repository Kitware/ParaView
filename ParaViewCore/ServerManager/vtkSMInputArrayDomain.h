/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputArrayDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMInputArrayDomain - requires input has array of described type
// .SECTION Description
// vtkSMInputArrayDomain requires that the source proxy pointed by the
// property has an output with one or more arrays of specified type.
// Current restrictions include whether the array is part of point or
// cell data and whether it has a given number of components. The restriction
// can be overriden for point and cell properties if the global post filter
// conversion is turned on.
// These attributes are specified in the XML file. Valid XML attributes are:
// @verbatim
// * attribute_type - cell or point
// * number_of_components
// @endverbatim
// The attribute type can also be (optionally) obtained from a required
// property FieldDataSelection which has a value of
// vtkDataSet::POINT_DATA_FIELD or  vtkDataSet::CELL_DATA_FIELD.
// .SECTION See Also
// vtkSMDomain

#ifndef __vtkSMInputArrayDomain_h
#define __vtkSMInputArrayDomain_h

#include "vtkSMDomain.h"

// Needed to get around some header defining ANY as a macro
#ifdef ANY
# undef ANY
#endif

class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMInputArrayDomain : public vtkSMDomain
{
public:
  static vtkSMInputArrayDomain* New();
  vtkTypeMacro(vtkSMInputArrayDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. The input has to have one or more
  // arrays that match the requirements.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if input has one or more arrays that match the
  // requirements on  the given output port.
  int IsInDomain(vtkSMSourceProxy* proxy, int outputport=0);

  // Description:
  // Returns 1 if the array represented by the array information is
  // a valid field. The attribute type (point or cell) as well as the
  // number of components are checked for a match
  int IsFieldValid(vtkSMSourceProxy* proxy, int outputport,
    vtkPVArrayInformation* arrayInfo);
  int IsFieldValid(vtkSMSourceProxy* proxy, int outputport,
    vtkPVArrayInformation* arrayInfo, int bypass);

  // Description:
  // Set/get the attribute type. Valid values are: POINT, CELL, ANY.
  // Text representations are: point, cell, any.
  vtkSetMacro(AttributeType, unsigned char);
  vtkGetMacro(AttributeType, unsigned char);
  const char* GetAttributeTypeAsString();
  virtual void SetAttributeType(const char* type);

  // Description:
  // Set/get the required number of components. Set to 0 for
  // no check.
  vtkSetMacro(NumberOfComponents, int);
  vtkGetMacro(NumberOfComponents, int);

  /// Get/Set the application wide setting for automatic conversion of properties.
  /// Automatic conversion of properties allows conversion between cell and point
  /// based properties, and the extraction of vector components as scalar properties
  static void SetAutomaticPropertyConversion(bool);
  static bool GetAutomaticPropertyConversion();

  // Description:
  // Use this method to convert a vtkDataSet::FIELD_ASSOCIATION_* to
  // AttributeTypes enum.
  static int GetAttributeTypeFromFieldAssociation(int);

//BTX
  enum AttributeTypes
  {
    POINT = 0,
    CELL = 1,
    ANY = 2,
    VERTEX = 3,
    EDGE = 4,
    ROW = 5,
    NONE = 6,
    LAST_ATTRIBUTE_TYPE
  };
//ETX

protected:
  vtkSMInputArrayDomain();
  ~vtkSMInputArrayDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void ChildSaveState(vtkPVXMLElement* domainElement);

  int AttributeInfoContainsArray(vtkSMSourceProxy* proxy,
                                 int outputport,
                                 vtkPVDataSetAttributesInformation* attrInfo);
  int CheckForArray(vtkPVArrayInformation* arrayInfo,
                    vtkPVDataSetAttributesInformation* attrInfo);

  unsigned char AttributeType;
  int NumberOfComponents;

private:
  static bool AutomaticPropertyConversion;
  vtkSMInputArrayDomain(const vtkSMInputArrayDomain&); // Not implemented
  void operator=(const vtkSMInputArrayDomain&); // Not implemented
};

#endif
