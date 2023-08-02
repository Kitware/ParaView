// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMDisplaySizedImplicitPlaneRepresentationProxy
 * @brief   proxy for a display sized implicit plane representation
 *
 * Specialized proxy for display sized implicit planes. Overrides the default appearance
 * of VTK display sized implicit plane representation.
 */

#ifndef vtkSMDisplaySizedImplicitPlaneRepresentationProxy_h
#define vtkSMDisplaySizedImplicitPlaneRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMWidgetRepresentationProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMDisplaySizedImplicitPlaneRepresentationProxy
  : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMDisplaySizedImplicitPlaneRepresentationProxy* New();
  vtkTypeMacro(vtkSMDisplaySizedImplicitPlaneRepresentationProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMDisplaySizedImplicitPlaneRepresentationProxy();
  ~vtkSMDisplaySizedImplicitPlaneRepresentationProxy() override;

  void SendRepresentation() override;

private:
  vtkSMDisplaySizedImplicitPlaneRepresentationProxy(
    const vtkSMDisplaySizedImplicitPlaneRepresentationProxy&) = delete;
  void operator=(const vtkSMDisplaySizedImplicitPlaneRepresentationProxy&) = delete;
};

#endif
