// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPlotMatrixViewProxy
 * @brief   Proxy class for plot matrix view
 */

#ifndef vtkSMPlotMatrixViewProxy_h
#define vtkSMPlotMatrixViewProxy_h

#include "vtkClientServerStream.h"  // For CS stream methods.
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMContextViewProxy.h"

class vtkAbstractContextItem;

class VTKREMOTINGVIEWS_EXPORT vtkSMPlotMatrixViewProxy : public vtkSMContextViewProxy
{
public:
  static vtkSMPlotMatrixViewProxy* New();
  vtkTypeMacro(vtkSMPlotMatrixViewProxy, vtkSMContextViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the vtk plot matrix.
   */
  vtkAbstractContextItem* GetContextItem() override;

protected:
  void CreateVTKObjects() override;
  void ActivePlotChanged();

  void PostRender(bool) override;

  bool ActiveChanged;

  vtkSMPlotMatrixViewProxy();
  ~vtkSMPlotMatrixViewProxy() override;
  void SendAnimationPath();
  void AnimationTickEvent();

private:
  vtkSMPlotMatrixViewProxy(const vtkSMPlotMatrixViewProxy&) = delete;
  void operator=(const vtkSMPlotMatrixViewProxy&) = delete;
};

#endif
