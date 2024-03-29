// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMCameraConfigurationWriter
 * @brief   A writer for XML camera configuration.
 *
 *
 * A writer for XML camera configuration. Writes camera configuration files
 * using ParaView state file machinery.
 *
 * @sa
 * vtkSMCameraConfigurationReader, vtkSMProxyConfigurationWriter
 *
 * @par Thanks:
 * This class was contributed by SciberQuest Inc.
 */

#ifndef vtkSMCameraConfigurationWriter_h
#define vtkSMCameraConfigurationWriter_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMProxyConfigurationWriter.h"

class vtkSMProxy;

class VTKREMOTINGMISC_EXPORT vtkSMCameraConfigurationWriter : public vtkSMProxyConfigurationWriter
{
public:
  vtkTypeMacro(vtkSMCameraConfigurationWriter, vtkSMProxyConfigurationWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSMCameraConfigurationWriter* New();

  /**
   * Set the render view proxy to extract camera properties from.
   */
  void SetRenderViewProxy(vtkSMProxy* rvProxy);

protected:
  vtkSMCameraConfigurationWriter();
  ~vtkSMCameraConfigurationWriter() override;

  // Protect the superclass's SetProxy, clients are forced to use
  // SetRenderViewProxy
  void SetProxy(vtkSMProxy*) override { vtkErrorMacro("Use SetRenderViewProxy."); }

private:
  vtkSMCameraConfigurationWriter(const vtkSMCameraConfigurationWriter&) = delete;
  void operator=(const vtkSMCameraConfigurationWriter&) = delete;
};

#endif
