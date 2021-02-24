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
/**
 * @class   vtkSMInputArrayDomain
 * @brief   domain to ensure that input has required types
 * of arrays.
 *
 * vtkSMInputArrayDomain is a domain that can be used on a vtkSMInputProperty to
 * check if the pipeline input provides attribute arrays of the required types
 * e.g. if a filter can only work if the input data set has cell data arrays,
 * then one can use this domain.
 *
 * vtkSMInputArrayDomain also provides a mechanism to check if the attribute
 * arrays have a certain number of components.
 *
 * When enabled, ParaView supports automatic array conversion i.e. extracting
 * components or converting cell data to point data and vice-versa is done
 * implicitly. In that case, vtkSMInputArrayDomain's behavior also changes as
 * appropriate.
 *
 * Supported XML attributes:
 * \li \c attribute_type : (optional) value can be 'point', 'cell', 'field',
 *                         'vertex', 'edge', 'row', 'none', 'any-except-field', 'any'.
 *                         If not specified, 'any-except-field' is assumed. This
 *                         indicates the attribute type for acceptable arrays.
 * \li \c number_of_components : (optional) Indicates the number of components
 *                         required in arrays that are considered acceptable.
 *                         0 (default) indicates any number of components is acceptable.
 *                         A comma-separated list (e.g., "1" or "1,3,4") of component counts
 *                         limits acceptable arrays to those with a number of components that
 *                         appear in the list.
 * \li \c data_type: (optional) when specified must be the class name for
 *                         data type that this domain is applicable to. This is
 *                         useful when adding multiple input-array-domains a
 *                         property with different requirements based on the
 *                         type (see `("filters", "Contour")` filter, for example).
 *
 * This domain doesn't support any required properties (to help clean old
 * code, we print a warning if any required properties are specified).
 *
 * @attention
 * Prior to ParaView 5.0, attribute_type="any" meant all attributes excepting
 * field data. For being consistent with general understanding of "any", this
 * has been changed to include field data arrays since 5.0. Use
 * "any-except-field" for cases where the intention is to match any attribute arrays except
 * field data arrays.
*/

#ifndef vtkSMInputArrayDomain_h
#define vtkSMInputArrayDomain_h

#include "vtkDataObject.h"                  // needed for vtkDataObject::AttributeTypes
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

#include <vector> // Needed for vector

// Needed to get around some header defining ANY as a macro
#ifdef ANY
#undef ANY
#endif

class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMSourceProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMInputArrayDomain : public vtkSMDomain
{
public:
  static vtkSMInputArrayDomain* New();
  vtkTypeMacro(vtkSMInputArrayDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMProxyProperty which points
   * to a vtkSMSourceProxy. The input has to have one or more
   * arrays that match the requirements.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Returns true if input has one or more arrays that match the
   * requirements on  the given output port.
   */
  int IsInDomain(vtkSMSourceProxy* proxy, unsigned int outputport = 0);

  //@{
  /**
   * Get the attribute type. Valid values are defined in AttributeTypes which
   * map to vtkDataObject::AttributeTypes.
   */
  vtkGetMacro(AttributeType, int);
  const char* GetAttributeTypeAsString();
  //@}

  /**
   * Get the AcceptableNumberOfComponents vector
   * Empty or containing a zero means no check.
   */
  std::vector<int> GetAcceptableNumbersOfComponents() const;

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
    ANY_EXCEPT_FIELD = vtkDataObject::POINT_THEN_CELL,
    VERTEX = vtkDataObject::VERTEX,
    EDGE = vtkDataObject::EDGE,
    ROW = vtkDataObject::ROW,
    ANY = vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES,
    NUMBER_OF_ATTRIBUTE_TYPES = ANY + 1,
  };

  /**
   * Method to check if a particular attribute-type (\c attribute_type) will
   * be accepted by this domain with a required attribute type (\c required_type).
   * This takes into consideration the state of AutomaticePropertyConversion flag.
   * If a particular attribute_type is acceptable only because
   * AutomaticPropertyConversion is true, \c acceptable_as_type value will be set
   * to the attribute type that the particular attribute was automatically converted
   * to. e.g. is required_type = POINT and attribute_type is CELL and
   * AutomaticPropertyConversion is true, this method will return true and
   * acceptable_as_type will be set to POINT. In other cases, acceptable_as_type
   * is simply set to attribute_type.
   */
  static bool IsAttributeTypeAcceptable(
    int required_type, int attribute_type, int* acceptable_as_type = nullptr);

  /**
   * This method will check if the arrayInfo contain info about an acceptable array,
   * by checking its number of components against this domain acceptable
   * numbers of components. Note that it takes into account property conversion
   * This method return the accepted number of components to use.
   */
  int IsArrayAcceptable(vtkPVArrayInformation* arrayInfo);

  /**
   * Helper method find the first `vtkSMInputArrayDomain` on the `property` that
   * is acceptable based on the value of the property. The property is typically
   * `vtkSMInputProperty`. If none of the domains applicable, this will simply
   * return the first vtkSMInputArrayDomain that is found.
   */
  static vtkSMInputArrayDomain* FindApplicableDomain(vtkSMProperty* property);

protected:
  vtkSMInputArrayDomain();
  ~vtkSMInputArrayDomain() override;

  vtkSetMacro(AttributeType, int);
  void SetAttributeType(const char* type);

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  /**
   * Returns true if based on this->AttributeType, the specified \c
   * attributeType is acceptable to this domain.
   */
  bool IsAttributeTypeAcceptable(int attributeType);

  /**
   * Returns true if based on this->AutomaticPropertyConversion and
   * this->NumberOfComponents, an acceptable array can be found in the attrInfo.
   */
  bool HasAcceptableArray(vtkPVDataSetAttributesInformation* attrInfo);

  int AttributeType;
  std::vector<int> AcceptableNumbersOfComponents;
  std::string DataType;

private:
  static bool AutomaticPropertyConversion;
  vtkSMInputArrayDomain(const vtkSMInputArrayDomain&) = delete;
  void operator=(const vtkSMInputArrayDomain&) = delete;
};

#endif
