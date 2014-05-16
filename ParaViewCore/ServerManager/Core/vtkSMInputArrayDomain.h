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
// .NAME vtkSMInputArrayDomain - domain to ensure that input has required types
// of arrays.
// .SECTION Description
// vtkSMInputArrayDomain is domain that can be used on a vtkSMInputProperty to
// check if the pipeline input provides attribute arrays of the required types
// e.g. if a filter can only work if the input data set has cell data arrays,
// then one can use this domain.
//
// vtkSMInputArrayDomain also provides a mechanism to check if the attribute
// arrays have a certain number of components.
//
// When enabled, ParaView supports automatic array conversion i.e. extracting
// components or converting cell data to point data and vice-versa is done
// implicitly. In that case, vtkSMInputArrayDomain's behavior also changes as
// appropriate.
//
// Supported XML attributes:
// \li \c attribute_type : (optional) value can be 'point', 'cell', 'any',
//                         'vertex', 'edge', 'row', 'none'. If no specified,
//                         'any' is assumed. This indicates the attribute type
//                         for acceptable arrays. Note "any" implies all types
//                         of attribute data (thus doesn't include field data
//                         since it's not attribute data).
// \li \c number_of_components : (optional) 0 by default. If non-zero, indicates
//                         the component count for acceptable arrays.
//
// This domain doesn't support any required properties (to help clean old
// code, we print a warning if any required properties are specified).
#ifndef __vtkSMInputArrayDomain_h
#define __vtkSMInputArrayDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"
#include "vtkDataObject.h" // needed for vtkDataObject::AttributeTypes

// Needed to get around some header defining ANY as a macro
#ifdef ANY
# undef ANY
#endif

class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMSourceProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMInputArrayDomain : public vtkSMDomain
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
  int IsInDomain(vtkSMSourceProxy* proxy, unsigned int outputport=0);

  // Description:
  // Get the attribute type. Valid values are defined in AttributeTypes which
  // map to vtkDataObject::AttributeTypes. 
  vtkGetMacro(AttributeType, int);
  const char* GetAttributeTypeAsString();

  // Description:
  // Get the required number of components. Set to 0 for no check.
  vtkGetMacro(NumberOfComponents, int);

  /// Get/Set the application wide setting for automatic conversion of properties.
  /// Automatic conversion of properties allows conversion between cell and point
  /// based properties, and the extraction of vector components as scalar properties
  static void SetAutomaticPropertyConversion(bool);
  static bool GetAutomaticPropertyConversion();

  enum AttributeTypes
    {
    POINT = vtkDataObject::POINT,
    CELL = vtkDataObject::CELL,
    FIELD = vtkDataObject::FIELD,
    ANY = vtkDataObject::POINT_THEN_CELL,
    VERTEX = vtkDataObject::VERTEX,
    EDGE = vtkDataObject::EDGE,
    ROW = vtkDataObject::ROW,
    NUMBER_OF_ATTRIBUTE_TYPES = vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES
    };

  // Description:
  // Method to check if a particular attribute-type (\c attribute_type) will
  // be accepted by this domain with a required attribute type (\c required_type).
  // This takes into consideration the state of AutomaticePropertyConversion flag.
  // If a particular attribute_type is acceptable only because
  // AutomaticPropertyConversion is true, \c acceptable_as_type value will be set
  // to the attribute type that the particular attribute was automatically converted
  // to. e.g. is required_type = POINT and attribute_type is CELL and
  // AutomaticPropertyConversion is true, this method will return true and
  // acceptable_as_type will be set to POINT. In other cases, acceptable_as_type
  // is simply set to attribute_type.
  static bool IsAttributeTypeAcceptable(int required_type, int attribute_type,
    int *acceptable_as_type=NULL);

  // Description:
  // Method to check if a particular array is acceptable to a domain with the
  // specified required number of components (\c required_number_of_components).
  // This takes into consideration the state of AutomaticePropertyConversion flag.
  // If AutomaticePropertyConversion, required_numer_of_components == 1 and
  // the actual number of components in the array are >= 1, then this method
  // will return true. This method will return true if
  // required_number_of_components == 0 (i.e. no restriction of num. of components
  // is specified) or if required_number_of_components == num. of components
  // in the array.
  static bool IsArrayAcceptable(
    int required_number_of_components, vtkPVArrayInformation* arrayInfo);
  
protected:
  vtkSMInputArrayDomain();
  ~vtkSMInputArrayDomain();

  vtkSetMacro(NumberOfComponents, int);
  vtkSetMacro(AttributeType, int);
  void SetAttributeType(const char* type);

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  // Description:
  // Returns true if based on this->AttributeType, the specified \c
  // attributeType is acceptable to this domain.
  bool IsAttributeTypeAcceptable(int attributeType);

  // Description:
  // Returns true if based on this->AutomaticPropertyConversion and
  // this->NumberOfComponents, an acceptable array can be found in the attrInfo.
  bool HasAcceptableArray(vtkPVDataSetAttributesInformation* attrInfo);

  int AttributeType;
  int NumberOfComponents;
private:
  static bool AutomaticPropertyConversion;
  vtkSMInputArrayDomain(const vtkSMInputArrayDomain&); // Not implemented
  void operator=(const vtkSMInputArrayDomain&); // Not implemented
};

#endif
