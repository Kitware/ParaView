/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMCameraConfigurationWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxyConfigurationWriter.h"

class vtkSMProxy;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMCameraConfigurationWriter
  : public vtkSMProxyConfigurationWriter
{
public:
  vtkTypeMacro(vtkSMCameraConfigurationWriter, vtkSMProxyConfigurationWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkSMCameraConfigurationWriter* New();

  /**
   * Set the render view proxy to extract camera properties from.
   */
  void SetRenderViewProxy(vtkSMProxy* rvProxy);

protected:
  vtkSMCameraConfigurationWriter();
  ~vtkSMCameraConfigurationWriter();

  // Protect the superclass's SetProxy, clients are forced to use
  // SetRenderViewProxy
  void SetProxy(vtkSMProxy*) VTK_OVERRIDE { vtkErrorMacro("Use SetRenderViewProxy."); }

private:
  vtkSMCameraConfigurationWriter(const vtkSMCameraConfigurationWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMCameraConfigurationWriter&) VTK_DELETE_FUNCTION;
};

#endif
