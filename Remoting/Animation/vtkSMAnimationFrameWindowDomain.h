// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMAnimationFrameWindowDomain
 * @brief domain for animation frame window.
 *
 * vtkSMAnimationFrameWindowDomain is used on the "FrameWindow" property on the
 * "SaveAnimation" proxy. It's a int-range, where the range is determined using
 * the animation scene's play mode and frame rate.
 */

#ifndef vtkSMAnimationFrameWindowDomain_h
#define vtkSMAnimationFrameWindowDomain_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMIntRangeDomain.h"

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimationFrameWindowDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMAnimationFrameWindowDomain* New();
  vtkTypeMacro(vtkSMAnimationFrameWindowDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to setup domain range values based on required properties
   * supported by this domain.
   */
  void Update(vtkSMProperty*) override;

protected:
  vtkSMAnimationFrameWindowDomain();
  ~vtkSMAnimationFrameWindowDomain() override;

private:
  vtkSMAnimationFrameWindowDomain(const vtkSMAnimationFrameWindowDomain&) = delete;
  void operator=(const vtkSMAnimationFrameWindowDomain&) = delete;
};

#endif
