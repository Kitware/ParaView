/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveViewHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAdaptiveViewHelper
// .SECTION Description
// Helper used to provide access to the render view proxy from the interactor.

#ifndef __vtkSMAdaptiveViewHelper_h
#define __vtkSMAdaptiveViewHelper_h

#include "vtkPVRenderViewProxy.h"

class vtkSMAdaptiveViewProxy;

class VTK_EXPORT vtkSMAdaptiveViewHelper : public vtkPVRenderViewProxy
{
public:
  static vtkSMAdaptiveViewHelper* New();
  vtkTypeMacro(vtkSMAdaptiveViewHelper, vtkPVRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Implemeting API from vtkPVRenderViewProxy.
  virtual void Render();
  virtual void EventuallyRender();
  virtual vtkRenderWindow* GetRenderWindow();

  // no reference counting.
  void SetAdaptiveView(vtkSMAdaptiveViewProxy* proxy)
    { this->AdaptiveView = proxy; }
  vtkGetObjectMacro(AdaptiveView, vtkSMAdaptiveViewProxy);
//BTX
protected:
  vtkSMAdaptiveViewHelper();
  ~vtkSMAdaptiveViewHelper();

  vtkSMAdaptiveViewProxy* AdaptiveView;
private:
  vtkSMAdaptiveViewHelper(const vtkSMAdaptiveViewHelper&); // Not implemented
  void operator=(const vtkSMAdaptiveViewHelper&); // Not implemented
//ETX
};

#endif

