// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class  vtkSMPrismThresholdRangeDomain
 * @brief  A domain that extracts the threshold range for each axis in the PrismView.
 *
 * Example usage is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * <DoubleVectorProperty command="SetLowerThresholdX"
 *                       default_values="0"
 *                       name="LowerThresholdX"
 *                       label="Lower Threshold"
 *                       number_of_elements="1">
 *   <PrismThresholdRangeDomain name="range" default_mode="min" axis_id="2">
 *     <RequiredProperties>
 *       <Property function="Bounds"
 *                 name="PrismBounds"/>
 *     </RequiredProperties>
 *   </PrismThresholdRangeDomain>
 * </DoubleVectorProperty>
 * </SourceProxy>
 * @endcode
 */
#ifndef vtkSMPrismThresholdRangeDomain_h
#define vtkSMPrismThresholdRangeDomain_h

#include "vtkPrismServerManagerModule.h"
#include "vtkSMArrayRangeDomain.h"

class VTKPRISMSERVERMANAGER_EXPORT vtkSMPrismThresholdRangeDomain : public vtkSMArrayRangeDomain
{
public:
  static vtkSMPrismThresholdRangeDomain* New();
  vtkTypeMacro(vtkSMPrismThresholdRangeDomain, vtkSMArrayRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the range based on the scalar range of the axis in the prism view. This requires Bounds
   * (vtkSMDoubleVectorProperty).
   */
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMPrismThresholdRangeDomain();
  ~vtkSMPrismThresholdRangeDomain() override;

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  int AxisId = -1;

private:
  vtkSMPrismThresholdRangeDomain(const vtkSMPrismThresholdRangeDomain&) = delete;
  void operator=(const vtkSMPrismThresholdRangeDomain&) = delete;
};

#endif // vtkSMPrismThresholdRangeDomain_h
