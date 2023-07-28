// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPSWriterProxy
 * @brief   proxy for the parallel-serial writer.
 *
 * vtkSMPSWriterProxy is the proxy for all vtkParallelSerialWriter
 * objects. It is responsible of setting the internal writer that is
 * configured as a sub-proxy.
 */

#ifndef vtkSMPSWriterProxy_h
#define vtkSMPSWriterProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMPWriterProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPSWriterProxy : public vtkSMPWriterProxy
{
public:
  static vtkSMPSWriterProxy* New();
  vtkTypeMacro(vtkSMPSWriterProxy, vtkSMPWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMPSWriterProxy();
  ~vtkSMPSWriterProxy() override;

private:
  vtkSMPSWriterProxy(const vtkSMPSWriterProxy&) = delete;
  void operator=(const vtkSMPSWriterProxy&) = delete;
};

#endif
