// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSICompoundSourceProxy
 *
 * vtkSICompoundSourceProxy is the server-side helper for a
 * vtkSMCompoundSourceProxy.
 * It provides the mapping to the exposed output port to the underneath
 * internal sub-proxy.
 */

#ifndef vtkSICompoundSourceProxy_h
#define vtkSICompoundSourceProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class vtkAlgorithm;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSICompoundSourceProxy : public vtkSISourceProxy
{
public:
  static vtkSICompoundSourceProxy* New();
  vtkTypeMacro(vtkSICompoundSourceProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the vtkAlgorithmOutput for an output port, if valid.
   */
  vtkAlgorithmOutput* GetOutputPort(int port) override;

protected:
  vtkSICompoundSourceProxy();
  ~vtkSICompoundSourceProxy() override;

  /**
   * Read xml-attributes.
   */
  bool ReadXMLAttributes(vtkPVXMLElement* element) override;

  /**
   * Create the output ports and add post filters for each output port.
   */
  bool CreateOutputPorts() override;

private:
  vtkSICompoundSourceProxy(const vtkSICompoundSourceProxy&) = delete;
  void operator=(const vtkSICompoundSourceProxy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
