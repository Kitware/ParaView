/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFieldDataDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMFieldDataDomain
 * @brief enumeration domain to select array association or attribute type.
 *
 * vtkSMFieldDataDomain can be used on a vtkSMIntVectorProperty that is intended
 * to specify the attribute type (`vtkDataObject::AttributeTypes`) or field association
 * (`vtkDataObject::FieldAssociations`).
 *
 * By default, `vtkDataObject::Field` (or vtkDataObject::FIELD_ASSOCIATION_NONE) is
 * not added to the domain. If you want to support field data, then use `enable_field_data="1"`
 * attribute in the XML configuration.
 *
 * By default, the domain is assumed to represent field
 * associations and hence the enumeration text is populated as "Point Data",
 * "Cell Data", etc. To use element types for text, add attribute
 * `use_element_types="1"` to the XML. In that case, the domain is populated
 * with "Point", "Cell", etc.
 *
 * @section DefaultValue Selecting the default value
 *
 * vtkSMFieldDataDomain picks the default value to be the first attribute type that has non empty
 * arrays
 * and non-zero tuples. If all attributes have no tuples, then the first attribute with non empty
 * arrays is selected. If all attributes have no arrays, then vtkSMEnumerationDomain
 * picks the default. For this to work, the domain must be provided an `Input` property as a
 * required property for function "Input".
 *
 * @section DeprecatedUsage Deprecated Usage
 *
 * Previously (5.6 and earlier), vtkSMFieldDataDomain was added to vtkSMStringVectorProperty
 * instances that allowed user to choose the array to process. This is no longer needed
 * or supported. Simply remove the vtkSMFieldDataDomain from the XML for such properties.
 *
 * The domain provided ability to limit the attribute types to the data type of the input dataset.
 * This was clumsy since it did not correctly handle cases where data type changes or
 * non empty attribute types changed. Hence we've dropped support for that. Simply remove
 * `disable_update_domain_entries` and `force_point_cell_data` attributes from the XML
 * for this domain since they are no longer supported.
 */

#ifndef vtkSMFieldDataDomain_h
#define vtkSMFieldDataDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMEnumerationDomain.h"

class vtkPVDataInformation;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMFieldDataDomain : public vtkSMEnumerationDomain
{
public:
  static vtkSMFieldDataDomain* New();
  vtkTypeMacro(vtkSMFieldDataDomain, vtkSMEnumerationDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to ensure that the property's default value is valid for the
   * enumeration, if not it will be set to the first enumeration value.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

  /**
   * Convenience method to return the string for an attribute type. Will return nullptr
   * for unsupported or invalid type.
   */
  static const char* GetAttributeTypeAsString(int attrType);

  /**
   * Convenience method to return the string for an element type. Will return nullptr
   * for unsupported or invalid type.
   */
  static const char* GetElementTypeAsString(int attrType);

  /**
   * Updates the available field data based on the input dataset type, if possible.
   * The provided property is not used.
   */
  void Update(vtkSMProperty* property) override;

protected:
  vtkSMFieldDataDomain();
  ~vtkSMFieldDataDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem) override;

  // When true, "Field Data" option is added to the domain.
  bool EnableFieldDataSelection;

  bool UseElementTypes;

private:
  vtkSMFieldDataDomain(const vtkSMFieldDataDomain&) = delete;
  void operator=(const vtkSMFieldDataDomain&) = delete;

  int ComputeDefaultValue(int currentValue);
};

#endif
