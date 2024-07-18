// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMDataTypeQueryDomain
 * @brief   A domain for getting the input data type and setting it
 *
 * This domain is used to query the data type of the input to the
 * property and set it as the default value and during the update.
 *
 * Example usage is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * <IntVectorProperty command="SetPlaceHolderDataType"
 *                    default_values="38"
 *                    name="PlaceHolderDataType"
 *                    panel_visibility="never"
 *                    number_of_elements="1">
 *    <DataTypeQueryDomain name="data_type_query">
 *     <RequiredProperties>
 *       <Property function="Input"
 *                 name="Input"/>
 *     </RequiredProperties>
 *   </DataTypeQueryDomain>
 * </IntVectorProperty>
 * </SourceProxy>
 * @endcode
 *
 * @sa
 * vtkSMDomain vtkSMIntVectorProperty
 */

#ifndef vtkSMDataTypeQueryDomain_h
#define vtkSMDataTypeQueryDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDataTypeQueryDomain : public vtkSMDomain
{
public:
  static vtkSMDataTypeQueryDomain* New();
  vtkTypeMacro(vtkSMDataTypeQueryDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the property is a vtkSMIntVectorProperty.
   * Return 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update(vtkSMProperty*) override;

  /**
   * Set Default value.
   */
  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

protected:
  vtkSMDataTypeQueryDomain();
  ~vtkSMDataTypeQueryDomain() override;

private:
  vtkSMDataTypeQueryDomain(const vtkSMDataTypeQueryDomain&) = delete;
  void operator=(const vtkSMDataTypeQueryDomain&) = delete;

  int InputDataType = -1;

  void OnDomainModified();
};

#endif // vtkSMDataTypeQueryDomain_h
