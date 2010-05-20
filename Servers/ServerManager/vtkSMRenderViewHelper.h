/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderViewHelper
// .SECTION Description
// Helper used to provide access to the render view proxy from the interactor.

#ifndef __vtkSMRenderViewHelper_h
#define __vtkSMRenderViewHelper_h

#include "vtkPVRenderViewProxy.h"

class vtkSMRenderViewProxy;

class VTK_EXPORT vtkSMRenderViewHelper : public vtkPVRenderViewProxy
{
public:
  static vtkSMRenderViewHelper* New();
  vtkTypeMacro(vtkSMRenderViewHelper, vtkPVRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Implemeting API from vtkPVRenderViewProxy.
  virtual void Render();
  virtual void EventuallyRender();
  virtual vtkRenderWindow* GetRenderWindow();

  // no reference counting.
  void SetRenderViewProxy(vtkSMRenderViewProxy* proxy)
    { this->RenderViewProxy = proxy; }
  vtkGetObjectMacro(RenderViewProxy, vtkSMRenderViewProxy);
//BTX
protected:
  vtkSMRenderViewHelper();
  ~vtkSMRenderViewHelper();

  vtkSMRenderViewProxy* RenderViewProxy;
private:
  vtkSMRenderViewHelper(const vtkSMRenderViewHelper&); // Not implemented
  void operator=(const vtkSMRenderViewHelper&); // Not implemented
//ETX
};

#endif

