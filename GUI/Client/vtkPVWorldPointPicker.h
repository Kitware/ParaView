/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWorldPointPicker.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWorldPointPicker - Picker to use with parallel compositing.
// .SECTION Description
// vtkPVWorldPointPicker uses the compositied z buffer to select the world point.

// .SECTION See Also
// vtkWorldPointPicker vtkPVRenderModule

#ifndef __vtkPVWorldPointPicker_h
#define __vtkPVWorldPointPicker_h

#include "vtkWorldPointPicker.h"

class vtkSMRenderModuleProxy;

class VTK_EXPORT vtkPVWorldPointPicker : public vtkWorldPointPicker
{
public:
  static vtkPVWorldPointPicker *New();
  vtkTypeRevisionMacro(vtkPVWorldPointPicker,vtkWorldPointPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // To use compositied z buffer value, we must have access to the compositer.
  virtual void SetRenderModuleProxy(vtkSMRenderModuleProxy* rm)
    { this->RenderModuleProxy = rm; }
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);

  // Description:
  // A pick method that uses composited zbuffer.
  int Pick(double selectionX, double selectionY, 
           double selectionZ, vtkRenderer *renderer);

protected:
  vtkPVWorldPointPicker();
  ~vtkPVWorldPointPicker();

  vtkSMRenderModuleProxy* RenderModuleProxy;

  vtkPVWorldPointPicker(const vtkPVWorldPointPicker&); // Not implemented
  void operator=(const vtkPVWorldPointPicker&); // Not implemented
};

#endif


