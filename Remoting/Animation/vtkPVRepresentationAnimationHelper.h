// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVRepresentationAnimationHelper
 * @brief   helper proxy used to animate
 * properties on the representations for any source.
 *
 * vtkPVRepresentationAnimationHelper is helper proxy used to animate
 * properties on the representations for any source. This makes is possible to
 * set up an animation cue that will affect properties on all representations
 * for a source without directly referring to the representation proxies.
 */

#ifndef vtkPVRepresentationAnimationHelper_h
#define vtkPVRepresentationAnimationHelper_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class VTKREMOTINGANIMATION_EXPORT vtkPVRepresentationAnimationHelper : public vtkSMProxy
{
public:
  static vtkPVRepresentationAnimationHelper* New();
  vtkTypeMacro(vtkPVRepresentationAnimationHelper, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Don't use directly. Use the corresponding properties instead.
   */
  void SetVisibility(int);
  void SetOpacity(double);
  void SetSourceProxy(vtkSMProxy* proxy);
  ///@}

protected:
  vtkPVRepresentationAnimationHelper();
  ~vtkPVRepresentationAnimationHelper() override;

  vtkWeakPointer<vtkSMProxy> SourceProxy;

private:
  vtkPVRepresentationAnimationHelper(const vtkPVRepresentationAnimationHelper&) = delete;
  void operator=(const vtkPVRepresentationAnimationHelper&) = delete;
};

#endif
