// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMImplicitPlaneRepresentationProxy
 * @brief   proxy for a implicit plane representation
 *
 * Specialized proxy for implicit planes. Overrides the default appearance
 * of VTK implicit plane representation.
 */

#ifndef vtkSMImplicitPlaneRepresentationProxy_h
#define vtkSMImplicitPlaneRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMWidgetRepresentationProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMImplicitPlaneRepresentationProxy
  : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMImplicitPlaneRepresentationProxy* New();
  vtkTypeMacro(vtkSMImplicitPlaneRepresentationProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMImplicitPlaneRepresentationProxy();
  ~vtkSMImplicitPlaneRepresentationProxy() override;

  void SendRepresentation() override;

private:
  vtkSMImplicitPlaneRepresentationProxy(const vtkSMImplicitPlaneRepresentationProxy&) = delete;
  void operator=(const vtkSMImplicitPlaneRepresentationProxy&) = delete;
};

#endif
