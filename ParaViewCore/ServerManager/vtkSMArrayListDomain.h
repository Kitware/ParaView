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
// * key_location / key_name / key_strategy:
//      those tree attributes are related to InformationKey of the array.
//      key_location/key_name are the location and name of the given InformationKey
//      key_strategy specifies if this specific key is needed to be in the domain
//      or if this key is rejected. One of need_key or reject_key.
//      if nothing is specified, the default is to add a vtkAbstractArray::GUI_HIDE
//      key, with the reject_key strategy, so that arrays that have this InformationKey
//      are not visible in the user interface.
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
#include "vtkStdString.h" // needed for vtkStdString.

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkPVArrayInformation;

//BTX
struct vtkSMArrayListDomainInternals;
//ETX

class VTK_EXPORT vtkSMArrayListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArrayListDomain* New();
  vtkTypeMacro(vtkSMArrayListDomain, vtkSMStringListDomain);
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
  // Get desired association of the current domain
  int GetDomainAssociation(unsigned int idx);

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

  //BTX
  // This enum represents the possible strategies associated
  // with a given InformationKey :
  // NEED_KEY means that if the array will be in the domain only if
  // it does contains the given information key in its information.
  // REJECT_KEY means that if the array will be in the domain only if
  // it does NOT contains the given information key in its information.
  enum InformationKeyStrategy
  {
    NEED_KEY,
    REJECT_KEY
  };
  //ETX

  // Description:
  // Adds a new InformationKey to the domain.
  // The default strategy is NEED_KEY if none is specified.
  // If no InformationKey is specified in the xml, the default
  // behavior is to create a rejected key vtkAbstractArray::GUI_HIDE
  virtual unsigned int AddInformationKey(const char* location, const char *name, int strategy);
  virtual unsigned int AddInformationKey(const char* location, const char *name)
  {
    return this->AddInformationKey(location, name, vtkSMArrayListDomain::NEED_KEY);
  }

  // Description:
  // Removes an InformationKey from this domain.
  unsigned int RemoveInformationKey(const char* location, const char *name);

  // Description:
  // Returns the number of InformationKeys in this domain.
  unsigned int GetNumberOfInformationKeys();

  //Description:
  // Removes all InformationKeys from this domain.
  void RemoveAllInformationKeys();

  // Description:
  // Returns the location/name/strategy of a given InformationKey
  const char* GetInformationKeyLocation(unsigned int);
  const char* GetInformationKeyName(unsigned int);
  int GetInformationKeyStrategy(unsigned int);

  // Description:
  // return 1 if the InformationKeys of this vtkPVArrayInformation
  // fullfill the requirements of the InformationKey in this Domain.
  // returns 0 on failure.
  int CheckInformationKeys(vtkPVArrayInformation* arrayInfo);


  // Description:
  // returns the mangled name for the component index that is passed in.
  //
  static vtkStdString CreateMangledName(vtkPVArrayInformation *arrayInfo, int component);

  // Description:
  // returns the mangled name for the component index that is passed in.
  //
  static vtkStdString ArrayNameFromMangledName(const char* name);
  static int ComponentIndexFromMangledName(vtkPVArrayInformation *info, const char* name);



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
                 int association, int domainAssociation=-1);

  // Description:
  // Adds a new array to the domain. This internally calls add string. If the \c
  // iad tells us that the number of components required==1 and the array has
  // more than 1 component and
  // vtkSMInputArrayDomain::GetAutomaticPropertyConversion() is true, then the
  // array is spilt into individual component and added (with name mangled using
  // the component names).
  // Returns the index for the array. If the array was split into components,
  // then returns the index of the string for the array magnitude.
  unsigned int AddArray(vtkPVArrayInformation* arrayinfo, int association, int domainAssociation,
    vtkSMInputArrayDomain* iad);

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
