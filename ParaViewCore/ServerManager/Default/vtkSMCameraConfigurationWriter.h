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
// .NAME vtkSMCameraConfigurationWriter - A writer for XML camera configuration.
//
// .SECTION Description
// A writer for XML camera configuration. Writes camera configuration files
// using ParaView state file machinery.
//
// .SECTION See Also
// vtkSMCameraConfigurationReader, vtkSMProxyConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSMCameraConfigurationWriter_h
#define __vtkSMCameraConfigurationWriter_h

#include "vtkSMProxyConfigurationWriter.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMCameraConfigurationWriter : public vtkSMProxyConfigurationWriter
{
public:
  vtkTypeMacro(vtkSMCameraConfigurationWriter,vtkSMProxyConfigurationWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMCameraConfigurationWriter *New();

  // Description:
  // Set the render view proxy to extract camera properties from.
  void SetRenderViewProxy(vtkSMProxy *rvProxy);

protected:
  vtkSMCameraConfigurationWriter();
  ~vtkSMCameraConfigurationWriter();

  // Protect the superclass's SetProxy, clients are forced to use
  // SetRenderViewProxy
  void SetProxy(vtkSMProxy *){ vtkErrorMacro("Use SetRenderViewProxy."); }

private:
  vtkSMCameraConfigurationWriter(const vtkSMCameraConfigurationWriter&);  // Not implemented.
  void operator=(const vtkSMCameraConfigurationWriter&);  // Not implemented.
};

#endif

