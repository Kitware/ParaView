// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPrismTableArraysDomain
 * @brief   domain for prism table arrays
 *
 * vtkSMPrismTableArraysDomain is a domain for prism table arrays that given the table id
 * and a flat array with all the array names, returns the array names for the given table id.
 *
 * Example usage is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * <StringVectorProperty name="ZArray"
 *                       command="SetZArray"
 *                       number_of_elements="1">
 *   <PrismTableArraysDomain name="prism_array_list" default_array_id="2">
 *     <RequiredProperties>
 *       <Property function="FlatArraysOfTables" name="FlatArraysOfTables"/>
 *       <Property function="TableId" name="TableId"/>
 *     </RequiredProperties>
 *   </PrismTableArraysDomain>
 * </StringVectorProperty>
 * </SourceProxy>
 * @endcode
 */
#ifndef vtkSMPrismTableArraysDomain_h
#define vtkSMPrismTableArraysDomain_h

#include "vtkPrismServerManagerModule.h" // For export macro
#include "vtkSMStringListDomain.h"

class VTKPRISMSERVERMANAGER_EXPORT vtkSMPrismTableArraysDomain : public vtkSMStringListDomain
{
public:
  static vtkSMPrismTableArraysDomain* New();
  vtkTypeMacro(vtkSMPrismTableArraysDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update(vtkSMProperty*) override;

  /**
   * Set the property's default value based on the domain. It's controlled by DefaultArray.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMPrismTableArraysDomain();
  ~vtkSMPrismTableArraysDomain() override;

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  int DefaultArrayId = 0;

private:
  vtkSMPrismTableArraysDomain(const vtkSMPrismTableArraysDomain&) = delete;
  void operator=(const vtkSMPrismTableArraysDomain&) = delete;
};

#endif // vtkSMPrismTableArraysDomain_h
