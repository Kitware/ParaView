// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPWriterProxy
 * @brief   proxy for a VTK writer that supports parallel
 * writing.
 *
 * vtkSMPWriterProxy is the proxy for all writers that can write in parallel.
 * The only extra functionality provided by this class is that it sets up the
 * piece information on the writer.
 */

#ifndef vtkSMPWriterProxy_h
#define vtkSMPWriterProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMWriterProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPWriterProxy : public vtkSMWriterProxy
{
public:
  static vtkSMPWriterProxy* New();
  vtkTypeMacro(vtkSMPWriterProxy, vtkSMWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMPWriterProxy();
  ~vtkSMPWriterProxy() override;

private:
  vtkSMPWriterProxy(const vtkSMPWriterProxy&) = delete;
  void operator=(const vtkSMPWriterProxy&) = delete;
};

#endif
