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
 * @class   vtkSMFieldDataDomain
 * @brief   enumeration with point and cell data entries
 *
 * vtkSMFieldDataDomain is a sub-class vtkSMEnumerationDomain that looks at
 * the input in Update() and populates the entry list based on whether
 * there are valid arrays in point/cell/vertex/edge/row data.
 * At most it consists of two
 * entries: ("Point Data", vtkDataObject::FIELD_ASSOCIATION_POINTS) and
 * ("Cell Data",  vtkDataObject::FIELD_ASSOCIATION_CELLS) in case of vtkDataSet
 * subclasses
 * OR
 * ("Vertex Data", vtkDataObject::FIELD_ASSOCIATION_VERTICES) and
 * ("Edge Data", vtkDataObject::FIELD_ASSOCIATION_EDGES) in case of vtkGraph and
 * subclasses
 * OR
 * ("Row Data", vtkDataObject::FIELD_ASSOCIATION_ROWS) in case of vtkTable and
 * subclasses.
 * It requires Input (vtkSMProxyProperty) property.
 * If attribute "disable_update_domain_entries" is set to true (false by
 * default),
 * then the domain values will not changed based on input field availability.
 * Only the default value setting will be affected by that.
 * @sa
 * vtkSMEnumerationDomain vtkSMProxyProperty
*/

#ifndef vtkSMFieldDataDomain_h
#define vtkSMFieldDataDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMEnumerationDomain.h"

class vtkPVDataInformation;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMFieldDataDomain : public vtkSMEnumerationDomain
{
public:
  static vtkSMFieldDataDomain* New();
  vtkTypeMacro(vtkSMFieldDataDomain, vtkSMEnumerationDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Check the input and appropriate fields (point data or cell data)
   * to the enumeration. This uses the Input property with a
   * vtkSMInputArrayDomain.
   */
  virtual void Update(vtkSMProperty* prop) VTK_OVERRIDE;

  /**
   * Overridden to ensure that the property's default value is valid for the
   * enumeration, if not it will be set to the first enumeration value.
   */
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) VTK_OVERRIDE;

protected:
  vtkSMFieldDataDomain();
  ~vtkSMFieldDataDomain();

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem) VTK_OVERRIDE;

  // When true, "Field Data" option is added to the domain.
  bool EnableFieldDataSelection;

  // When true, we don't update the available list of attributes based on what's
  // actually available in the input (false by default).
  bool DisableUpdateDomainEntries;

  // When true, "Point Data" and "Cell Data" is included to the domain even
  // if they don't have any properties. This is used by the spreadsheet
  // view.( false by default )
  bool ForcePointAndCellDataSelection;

private:
  // Used by SetDefaultValues.
  int DefaultValue;

  /**
   * Utility functions called by Update()
   */
  void UpdateDomainEntries(int acceptable_association, vtkPVDataInformation* dataInfo);

  vtkSMFieldDataDomain(const vtkSMFieldDataDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMFieldDataDomain&) VTK_DELETE_FUNCTION;
};

#endif
