/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismIsSimulationDataDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPrismIsSimulationDataDomain
 * @brief   domain for checking if the input is simulation data
 *
 * This domain checks if the input is simulation data. If it is, then the
 * IsSimulationData property is set to 1, otherwise it is set to 0.
 *
 * Example usage is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * <IntVectorProperty command="SetIsSimulationData"
 *                    default_values="0"
 *                    name="IsSimulationData"
 *                    number_of_elements="1">
 *    <PrismIsSimulationDataDomain name="bool">
 *     <RequiredProperties>
 *       <Property function="Input"
 *                 name="Input"/>
 *     </RequiredProperties>
 *   </PrismIsSimulationDataDomain>
 * </IntVectorProperty>
 * </SourceProxy>
 * @endcode
 */
#ifndef vtkSMPrismIsSimulationDataDomain_h
#define vtkSMPrismIsSimulationDataDomain_h

#include "vtkPrismServerManagerModule.h" // for export macro
#include "vtkSMBooleanDomain.h"

class VTKPRISMSERVERMANAGER_EXPORT vtkSMPrismIsSimulationDataDomain : public vtkSMBooleanDomain
{
public:
  static vtkSMPrismIsSimulationDataDomain* New();
  vtkTypeMacro(vtkSMPrismIsSimulationDataDomain, vtkSMBooleanDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update property by checking if the Input property has a FieldData PrismData array.
   */
  void Update(vtkSMProperty*) override;

  /**
   * Set Default value.
   */
  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

protected:
  vtkSMPrismIsSimulationDataDomain();
  ~vtkSMPrismIsSimulationDataDomain() override;

  bool IsSimulationData = false;

private:
  vtkSMPrismIsSimulationDataDomain(const vtkSMPrismIsSimulationDataDomain&) = delete;
  void operator=(const vtkSMPrismIsSimulationDataDomain&) = delete;
};

#endif // vtkSMPrismIsSimulationDataDomain_h
