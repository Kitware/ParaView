// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMCameraConfigurationReader
 * @brief   A reader for XML camera configuration.
 *
 *
 * A reader for XML camera configuration. Reades camera configuration files.
 * written by the vtkSMCameraConfigurationWriter.
 *
 * @sa
 * vtkSMCameraConfigurationWriter, vtkSMProxyConfigurationReader
 *
 * @par Thanks:
 * This class was contributed by SciberQuest Inc.
 */

#ifndef vtkSMCameraConfigurationReader_h
#define vtkSMCameraConfigurationReader_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMProxyConfigurationReader.h"

class vtkSMProxy;
class vtkPVXMLElement;

class VTKREMOTINGMISC_EXPORT vtkSMCameraConfigurationReader : public vtkSMProxyConfigurationReader
{
public:
  vtkTypeMacro(vtkSMCameraConfigurationReader, vtkSMProxyConfigurationReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSMCameraConfigurationReader* New();

  /**
   * Set the render view proxy to extract camera properties from.
   */
  void SetRenderViewProxy(vtkSMProxy* rvProxy);

  ///@{
  /**
   * Read the named file, and push the properties into the underying
   * managed render view proxy. This will make sure the renderview is
   * updated after the read.
   */
  int ReadConfiguration(const char* filename) override;
  int ReadConfiguration(vtkPVXMLElement* x) override;
  // unhide
  int ReadConfiguration() override { return this->Superclass::ReadConfiguration(); }
  ///@}

protected:
  vtkSMCameraConfigurationReader();
  ~vtkSMCameraConfigurationReader() override;

  // Protect the superclass's SetProxy, clients are forced to use
  // SetRenderViewProxy
  void SetProxy(vtkSMProxy*) override { vtkErrorMacro("Use SetRenderViewProxy."); }

private:
  vtkSMCameraConfigurationReader(const vtkSMCameraConfigurationReader&) = delete;
  void operator=(const vtkSMCameraConfigurationReader&) = delete;
};

#endif
