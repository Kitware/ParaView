// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMViewResolutionDomain_h
#define vtkSMViewResolutionDomain_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMIntRangeDomain.h"

class vtkSMViewLayoutProxy;
class vtkSMViewProxy;

/* @class vtkSMViewProxy
 * @brief domain for view (or layout) resolution.
 *
 * Int range domain that sets up the range based on the current view (or layout resolution).
 *
 */
class VTKREMOTINGVIEWS_EXPORT vtkSMViewResolutionDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMViewResolutionDomain* New();
  vtkTypeMacro(vtkSMViewResolutionDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to setup domain range values based on required properties
   * supported by this domain.
   */
  void Update(vtkSMProperty*) override;

protected:
  vtkSMViewResolutionDomain();
  ~vtkSMViewResolutionDomain() override;

  void GetLayoutResolution(vtkSMViewLayoutProxy* layout, int resolution[2]);
  void GetViewResolution(vtkSMViewProxy* view, int resolution[2]);

private:
  vtkSMViewResolutionDomain(const vtkSMViewResolutionDomain&) = delete;
  void operator=(const vtkSMViewResolutionDomain&) = delete;
};

#endif
