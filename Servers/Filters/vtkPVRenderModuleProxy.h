/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderModuleProxy - Forwards calls to the vtkPVRenderModule class
//
// .SECTION Description
// This class is used to allow separation between client GUI code and server code.
// It forwards calls to the vtkPVRenderModule class.
//
// .SECTION See Also
// vtkPVRenderViewProxy

#ifndef __vtkPVRenderModuleProxy_h
#define __vtkPVRenderModuleProxy_h

#include "vtkObject.h"

class VTK_EXPORT vtkPVRenderModuleProxy : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkPVRenderModuleProxy, vtkObject);

  // Description:
  // Forward these calls to an actual vtkPVRenderView in a sub class.
  virtual float GetZBufferValue(int x, int y) = 0;

protected:
  vtkPVRenderModuleProxy() {};
  ~vtkPVRenderModuleProxy() {};

private:
  vtkPVRenderModuleProxy(const vtkPVRenderModuleProxy&); // Not implemented
  void operator=(const vtkPVRenderModuleProxy&); // Not implemented
};

#endif
