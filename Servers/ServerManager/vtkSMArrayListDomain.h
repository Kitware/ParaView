/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArrayListDomain - list of arrays obtained from input
// .SECTION Description
// vtkSMArrayListDomain represents a domain consisting of array names
// obtained from an input. vtkSMArrayListDomain requires
// a property of class vtkSMProxyProperty which points to a
// vtkSMSourceProxy and contains a vtkSMInputArrayDomain. Only
// the first proxy and domain are used.
// Valid XML attributes are:
// @verbatim
// * attribute_type - one of:
//    - scalars
//    - vectors
//    - normals
//    - tcoords
//    - tensors
// * data_type - one or more of:
//    - VTK_BIT, VTK_CHAR, VTK_INT, VTK_FLOAT, VTK_DOUBLE,... etc etc or the equivalent integers
//      from vtkType.h
//    - VTK_VOID, and 0 are equivalent to not specifying, meaning any data type
//      is allowed
// * none_string - when specified, this string appears as the first entry in
//      the list and can be used to show "None", or "Not available" etc.
// @endverbatim
// Additionally, vtkSMArrayListDomain support 'default_values' attribute which
// specifies a string (only 1 string value is supported). When
// SetDefaultValues() is called, if the array name specified as 'default_values'
// is present in the domain, then that will be used, otherwise, it simply uses
// the first available array (which is default).
//
// Additionally, vtkSMArrayListDomain takes an  option required property with
// function "FieldDataSelection" which can be a vtkSMIntVectorProperty with a
// single value. If preset, this property's value is used to determine what type
// of field-data i.e. point-data, cell-data etc. is currently available.
// .SECTION See Also
// vtkSMDomain vtkSMProxyProperty vtkSMInputArrayDomain

#ifndef __vtkSMArrayListDomain_h
#define __vtkSMArrayListDomain_h

#include "vtkSMStringListDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;

//BTX
struct vtkSMArrayListDomainInternals;
//ETX

class VTK_EXPORT vtkSMArrayListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArrayListDomain* New();
  vtkTypeRevisionMacro(vtkSMArrayListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the string list based on the available arrays. Requires
  // a property of class vtkSMProxyProperty which points to a
  // vtkSMSourceProxy and contains a vtkSMInputArrayDomain. Only
  // the first proxy and domain are used.
  virtual void Update(vtkSMProperty* prop);

  // Description:
  // The DefaultElement is set during Update() using the "active
  // attribute" of the assigned AttributeType. For example,
  // if the AttributeType is set to SCALARS, DefaultElement is
  // set to the index of the array that is the active scalars
  // in the dataset.
  vtkGetMacro(DefaultElement, unsigned int);

  // Description:
  // Returns true if the array with the given idx is partial
  // false otherwise. See vtkPVArrayInformation for more information.
  int IsArrayPartial(unsigned int idx);

  // Description:
  // Get field association for the array.
  int GetFieldAssociation(unsigned int idx);

  // Description:
  // Return the attribute type. The values are listed in
  // vtkDataSetAttributes.h.
  vtkGetMacro(AttributeType, int);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  // Returns 1 if the domain updated the property.
  virtual int SetDefaultValues(vtkSMProperty*);

  // Description:
  // Adds a new string to the domain.
  unsigned int AddString(const char* string);

  // Description:
  // Removes a string from the domain.
  virtual int RemoveString(const char* string);

  // Description:
  // Removes all strings from the domain.
  virtual void RemoveAllStrings();

protected:
  vtkSMArrayListDomain();
  ~vtkSMArrayListDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  // Description:
  // Utility functions called by Update()
  void AddArrays(vtkSMSourceProxy* sp, 
                 int outputport,
                 vtkPVDataSetAttributesInformation* info, 
                 vtkSMInputArrayDomain* iad,
                 int association);
  void Update(vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad, int outputport);
  void Update(vtkSMProxyProperty* pp, vtkSMSourceProxy* sp, int outputport);
  void Update(vtkSMProxyProperty* pp);

  // Description:
  // Set to an attribute type defined in vtkDataSetAttributes.
  vtkSetMacro(AttributeType, int);

  vtkSetMacro(DefaultElement, unsigned int);

  int AttributeType;
  int Attribute;
  int Association;
  unsigned int DefaultElement;

  // Description:
  // InputDomainName refers to a input property domain that describes
  // the type of array is needed by this property.
  vtkGetStringMacro(InputDomainName);
  vtkSetStringMacro(InputDomainName);

  vtkSetStringMacro(NoneString);
  vtkGetStringMacro(NoneString);

  char* InputDomainName;
  char* NoneString;
private:
  vtkSMArrayListDomain(const vtkSMArrayListDomain&); // Not implemented
  void operator=(const vtkSMArrayListDomain&); // Not implemented

  vtkSMArrayListDomainInternals* ALDInternals;
};

#endif
