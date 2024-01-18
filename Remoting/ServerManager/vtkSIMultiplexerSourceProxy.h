// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSIMultiplexerSourceProxy
 * @brief vtkSIProxy subclass for vtkSMMultiplexerSourceProxy
 *
 * vtkSIMultiplexerSourceProxy is intended for use with
 * vtkSMMultiplexerSourceProxy. It adds API to activate a subproxy to act as the
 * data producer.
 */

#ifndef vtkSIMultiplexerSourceProxy_h
#define vtkSIMultiplexerSourceProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIMultiplexerSourceProxy : public vtkSISourceProxy
{
public:
  static vtkSIMultiplexerSourceProxy* New();
  vtkTypeMacro(vtkSIMultiplexerSourceProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called to select one of the subproxies as the active one.
   */
  void Select(vtkSISourceProxy* subproxy);

protected:
  vtkSIMultiplexerSourceProxy();
  ~vtkSIMultiplexerSourceProxy() override;

private:
  vtkSIMultiplexerSourceProxy(const vtkSIMultiplexerSourceProxy&) = delete;
  void operator=(const vtkSIMultiplexerSourceProxy&) = delete;
};

#endif
