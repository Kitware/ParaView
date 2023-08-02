// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMOrthographicSliceViewProxy
 *
 *
 */

#ifndef vtkSMOrthographicSliceViewProxy_h
#define vtkSMOrthographicSliceViewProxy_h

#include "vtkSMRenderViewProxy.h"
class vtkSMRepresentationProxy;

class VTKREMOTINGVIEWS_EXPORT vtkSMOrthographicSliceViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMOrthographicSliceViewProxy* New();
  vtkTypeMacro(vtkSMOrthographicSliceViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to forward the call to the internal root view proxy.
   */
  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

  /**
   * Overridden to set initial default slices when a representation is created.
   * Not sure that's the best way to do this, but leaving the logic unchanged in
   * this pass.
   */
  vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy* proxy, int outputPort) override;

protected:
  vtkSMOrthographicSliceViewProxy();
  ~vtkSMOrthographicSliceViewProxy() override;

  void InitDefaultSlices(vtkSMSourceProxy* source, int opport, vtkSMRepresentationProxy* repr);

  void CreateVTKObjects() override;
  void OnMouseWheelBackwardEvent(vtkObject*, unsigned long, void* calldata);
  void OnMouseWheelForwardEvent(vtkObject*, unsigned long, void* calldata);
  void OnPlacePointEvent(vtkObject*, unsigned long, void* calldata);

private:
  vtkSMOrthographicSliceViewProxy(const vtkSMOrthographicSliceViewProxy&) = delete;
  void operator=(const vtkSMOrthographicSliceViewProxy&) = delete;
};

#endif
