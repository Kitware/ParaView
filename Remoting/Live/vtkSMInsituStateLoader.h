// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMInsituStateLoader
 *
 *
 */

#ifndef vtkSMInsituStateLoader_h
#define vtkSMInsituStateLoader_h

#include "vtkRemotingLiveModule.h" //needed for exports
#include "vtkSMStateLoader.h"

class VTKREMOTINGLIVE_EXPORT vtkSMInsituStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMInsituStateLoader* New();
  vtkTypeMacro(vtkSMInsituStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMInsituStateLoader();
  ~vtkSMInsituStateLoader() override;

  /**
   * Overridden to try to reuse existing proxies as much as possible.
   */
  vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) override;

private:
  vtkSMInsituStateLoader(const vtkSMInsituStateLoader&) = delete;
  void operator=(const vtkSMInsituStateLoader&) = delete;
};

#endif
