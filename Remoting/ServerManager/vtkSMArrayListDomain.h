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
/**
 * @class   vtkSMArrayListDomain
 * @brief   list of arrays obtained from input
 *
 * vtkSMArrayListDomain is used on vtkSMStringVectorProperty when the values
 * on the property correspond to data arrays in the input.
 *
 * Supported Required Property Functions:
 * \li \c Input : (required) this point to a vtkSMInputProperty on the parent
 *                proxy. The value of this property provides the source that
 *                provides the data information to determine the available
 *                arrays.
 * \li \c FieldDataSelection : (optional) this points a vtkSMIntVectorProperty
 *                that provide the array association for accepted arrays as
 *                defined by vtkDataObject::FieldAssociations. If
 *                FieldDataSelection is missing, then the array association is
 *                determined using the vtkSMInputArrayDomain on the
 *                vtkSMInputProperty pointed by the required function \c Input.
 *                If the input property has multiple vtkSMInputArrayDomain
 *                types, you can identify the domain to use by using the
 *                \li input_domain_name XML attribute. If neither the
 *                FieldDataSelection is specified and no vtkSMInputArrayDomain
 *                is found, then this domain assumes that all array associations
 *                are valid.
 *
 * Supported XML attributes:
 * \li \c attribute_type : (optional) when specified, this is used to pick the
 *                default value in SetDefaultValues. This specifies the
 *                array-attribute type to pick as the default, if available e.g.
 *                if value is "Scalars", then by default the active scalar array
 *                will be picked, if available. Not to be confused with
 *                vtkDataObject::AttributeTypes, this corresponds to
 *                vtkDataSetAttributes::AttributeTypes.
 *                Accepted values are "Scalars", "Vectors", etc., as defined by
 *                vtkDataSetAttributes::AttributeNames.
 * \li \c data_type: (optional) when specified qualifies the acceptable arrays
 *                list to the types specified. Value can be one or more of
 *                VTK_BIT, VTK_CHAR, VTK_INT, VTK_FLOAT, VTK_DOUBLE,... etc.
 *                or the equivalent integers from vtkType.h.  VTK_VOID, and 0
 *                are equivalent to not specifying, meaning any data type.
 *                VTK_DATA_ARRAY can be used to limit to vtkDataArray
 *                subclasses.
 * \li \c none_string: (optional) when specified, this string appears as the
 *                first entry in the domain the list and can be used to show
 *                "None", or "Not available" etc.
 * \li \c key_location / \c key_name / \c key_strategy: (optional)
 *      those tree attributes are related to InformationKey of the array.
 *      key_location/key_name are the location and name of the given InformationKey
 *      key_strategy specifies if this specific key is needed to be in the domain
 *      or if this key is rejected. One of need_key or reject_key.
 *      if nothing is specified, the default is to add a vtkAbstractArray::GUI_HIDE
 *      key, with the reject_key strategy, so that arrays that have this InformationKey
 *      are not visible in the user interface.
*/

#ifndef vtkSMArrayListDomain_h
#define vtkSMArrayListDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkPVArrayInformation;

class vtkSMArrayListDomainInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMArrayListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArrayListDomain* New();
  vtkTypeMacro(vtkSMArrayListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the string list based on the available arrays. Requires
   * a property of class vtkSMProxyProperty which points to a
   * vtkSMSourceProxy and contains a vtkSMInputArrayDomain. Only
   * the first proxy and domain are used.
   */
  void Update(vtkSMProperty* prop) override;

  /**
   * Returns true if the array with the given idx is partial
   * false otherwise. See vtkPVArrayInformation for more information.
   */
  int IsArrayPartial(unsigned int idx);

  /**
   * Get field association for the array. When
   * vtkSMInputArrayDomain::AutomaticPropertyConversion is ON, this is not the
   * true association for a particular array, but what the target filter is
   * expecting. Thus use this to set the value on the property.
   * To correctly show icons in UI, use GetDomainAssociation().
   */
  int GetFieldAssociation(unsigned int idx);

  /**
   * Get the true field association for the array. This is same as
   * GetFieldAssociation() except when
   * vtkSMInputArrayDomain::AutomaticPropertyConversion is ON. In that case,
   * this may be different. e.g. let's say Pressure is a point array on the
   * input, however this filter only works with cell array. In that case, since
   * AutomaticPropertyConversion is ON, vtkPVPostFilter is going to
   * automatically convert the point array Pressure to a cell array for the
   * filter. Now in this case, the SetInputArrayToProcess property on the filter
   * must be set to ask a "cell" array named Pressure, despite the fact that
   * there's no such cell array. And the UI needs to show the "Pressure" as a
   * point array, since that's what the user is expecting. In this case,
   * GetFieldAssociation() is going to return "CELL" for the "Pressure", while
   * GetDomainAssociation() is going to return "POINT". Thus, use
   * GetFieldAssociation() for setting the property value, but use
   * GetDomainAssociation() for the icon.
   */
  int GetDomainAssociation(unsigned int idx);

  //@{
  /**
   * Return the attribute type. The values are listed in
   * vtkDataSetAttributes.h.
   */
  vtkGetMacro(AttributeType, int);
  //@}

  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   * Returns 1 if the domain updated the property.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

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

  //@{
  /**
   * Adds a new InformationKey to the domain.
   * The default strategy is NEED_KEY if none is specified.
   * If no InformationKey is specified in the xml, the default
   * behavior is to create a rejected key vtkAbstractArray::GUI_HIDE
   */
  virtual unsigned int AddInformationKey(const char* location, const char* name, int strategy);
  virtual unsigned int AddInformationKey(const char* location, const char* name)
  {
    return this->AddInformationKey(location, name, vtkSMArrayListDomain::NEED_KEY);
  }
  //@}

  /**
   * Removes an InformationKey from this domain.
   */
  unsigned int RemoveInformationKey(const char* location, const char* name);

  /**
   * Returns the number of InformationKeys in this domain.
   */
  unsigned int GetNumberOfInformationKeys();

  /**
   * Removes all InformationKeys from this domain.
   */
  void RemoveAllInformationKeys();

  //@{
  /**
   * Returns the location/name/strategy of a given InformationKey
   */
  const char* GetInformationKeyLocation(unsigned int);
  const char* GetInformationKeyName(unsigned int);
  int GetInformationKeyStrategy(unsigned int);
  //@}

  /**
   * returns the mangled name for the component index that is passed in.

   */
  static std::string CreateMangledName(vtkPVArrayInformation* arrayInfo, int component);

  //@{
  /**
   * returns the mangled name for the component index that is passed in.

   */
  static std::string ArrayNameFromMangledName(const char* name);
  static int ComponentIndexFromMangledName(vtkPVArrayInformation* info, const char* name);
  //@}

protected:
  vtkSMArrayListDomain();
  ~vtkSMArrayListDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  /**
   * HACK: Provides a temporary mechanism for subclasses to provide an
   * "additional" vtkPVDataInformation instance to get available arrays list
   * from.
   */
  virtual vtkPVDataInformation* GetExtraDataInformation() { return nullptr; }

  /**
   * Returns true if an array should be filtered out. This is typically used to
   * filter out arrays based on their names by subclasses.
   * This implementation always returns false, but subclasses may override.
   */
  virtual bool IsFilteredArray(vtkPVDataInformation* info, int association, const char* arrayName);

  //@{
  /**
   * Set to an attribute type defined in vtkDataSetAttributes.
   */
  vtkSetMacro(AttributeType, int);
  int AttributeType;
  //@}

  //@{
  /**
   * InputDomainName refers to a input property domain that describes
   * the type of array is needed by this property.
   */
  vtkGetStringMacro(InputDomainName);
  vtkSetStringMacro(InputDomainName);
  //@}

  char* InputDomainName;

  // Currently, used by vtkSMRepresentedArrayListDomain to avoid picking just an
  // arbitrary array for scalar coloring. Need to rethink how this should be
  // done cleanly.
  bool PickFirstAvailableArrayByDefault;

private:
  vtkSMArrayListDomain(const vtkSMArrayListDomain&) = delete;
  void operator=(const vtkSMArrayListDomain&) = delete;

  friend class vtkSMArrayListDomainInternals;
  vtkSMArrayListDomainInternals* ALDInternals;
};

#endif
