/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleProxyImplementation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderModuleProxyImplementation
// .SECTION Description
// This class is used to allow separation between client GUI code and server code.
// It forwards calls to the vtkPVRenderView class.

#ifndef __vtkPVRenderModuleProxyImplementation_h
#define __vtkPVRenderModuleProxyImplementation_h

#include "vtkPVRenderModuleProxy.h"

class vtkRenderer;
class vtkPVRenderModule;

class VTK_EXPORT vtkPVRenderModuleProxyImplementation : public vtkPVRenderModuleProxy
{
public:
  static vtkPVRenderModuleProxyImplementation *New();
  vtkTypeRevisionMacro(vtkPVRenderModuleProxyImplementation, vtkPVRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description:
  // Forward these calls to an actual vtkPVRenderModule in a sub class.
  virtual float GetZBufferValue(int x, int y);

  // Description:
  // Set/Get the PVRenderView.
  void SetPVRenderModule(vtkPVRenderModule *view);
  vtkGetObjectMacro(PVRenderModule, vtkPVRenderModule);
protected:
  vtkPVRenderModuleProxyImplementation();
  ~vtkPVRenderModuleProxyImplementation();
private:
  vtkPVRenderModuleProxyImplementation(const vtkPVRenderModuleProxyImplementation&); // Not implemented
  void operator=(const vtkPVRenderModuleProxyImplementation&); // Not implemented
  vtkPVRenderModule* PVRenderModule;
};

#endif
