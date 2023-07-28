// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMLightProxy
 * @brief   a configurable light proxy.
 *
 * vtkSMLightProxy is a configurable light. One or more can exist in a view.
 */

#ifndef vtkSMLightProxy_h
#define vtkSMLightProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkSMLightObserver;

class VTKREMOTINGVIEWS_EXPORT vtkSMLightProxy : public vtkSMProxy
{
public:
  static vtkSMLightProxy* New();
  vtkTypeMacro(vtkSMLightProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMLightProxy();
  ~vtkSMLightProxy() override;

  void CreateVTKObjects() override;

  void PropertyChanged();
  friend class vtkSMLightObserver;
  vtkSMLightObserver* Observer;

private:
  vtkSMLightProxy(const vtkSMLightProxy&) = delete;
  void operator=(const vtkSMLightProxy&) = delete;
};

#endif
