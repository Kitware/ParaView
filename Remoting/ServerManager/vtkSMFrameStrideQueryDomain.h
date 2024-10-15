// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMFrameStrideQueryDomain
 * @brief   A domain for getting the input data type and setting it
 *
 * This domain is used to query the frame stride of the animation scene to the
 * property and set it as the default value and during the update.
 *
 * Example usage is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * <IntVectorProperty command="SetFrameStride"
 *                    default_values="1"
 *                    name="FrameStride"
 *                    number_of_elements="1">
 *    <FrameStrideQueryDomain name="frame_stride_query">
 *     <RequiredProperties>
 *       <Property function="AnimationScene"
 *                 name="AnimationScene"/>
 *     </RequiredProperties>
 *   </FrameStrideQueryDomain>
 * </IntVectorProperty>
 * </SourceProxy>
 * @endcode
 *
 * @sa
 * vtkSMDomain vtkSMIntVectorProperty
 */

#ifndef vtkSMFrameStrideQueryDomain_h
#define vtkSMFrameStrideQueryDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMFrameStrideQueryDomain : public vtkSMDomain
{
public:
  static vtkSMFrameStrideQueryDomain* New();
  vtkTypeMacro(vtkSMFrameStrideQueryDomain, vtkSMDomain);
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
  vtkSMFrameStrideQueryDomain();
  ~vtkSMFrameStrideQueryDomain() override;

private:
  vtkSMFrameStrideQueryDomain(const vtkSMFrameStrideQueryDomain&) = delete;
  void operator=(const vtkSMFrameStrideQueryDomain&) = delete;

  int FrameStride = 1;

  void OnDomainModified();
};

#endif // vtkSMFrameStrideQueryDomain_h
