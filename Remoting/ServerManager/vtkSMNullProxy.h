// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMNullProxy
 * @brief   proxy with stands for nullptr object on the server.
 *
 * vtkSMNullProxy stands for a 0 on the server side.
 */

#ifndef vtkSMNullProxy_h
#define vtkSMNullProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMNullProxy : public vtkSMProxy
{
public:
  static vtkSMNullProxy* New();
  vtkTypeMacro(vtkSMNullProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMNullProxy();
  ~vtkSMNullProxy() override;

  void CreateVTKObjects() override;

private:
  vtkSMNullProxy(const vtkSMNullProxy&) = delete;
  void operator=(const vtkSMNullProxy&) = delete;
};

#endif
