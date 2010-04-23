/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingViewHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStreamingViewHelper
// .SECTION Description
// Helper used to provide access to the render view proxy from the interactor.

#ifndef __vtkSMStreamingViewHelper_h
#define __vtkSMStreamingViewHelper_h

#include "vtkPVRenderViewProxy.h"

class vtkSMStreamingViewProxy;

class VTK_EXPORT vtkSMStreamingViewHelper : public vtkPVRenderViewProxy
{
public:
  static vtkSMStreamingViewHelper* New();
  vtkTypeMacro(vtkSMStreamingViewHelper, vtkPVRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Implemeting API from vtkPVRenderViewProxy.
  virtual void Render();
  virtual void EventuallyRender();
  virtual vtkRenderWindow* GetRenderWindow();

  // no reference counting.
  void SetStreamingView(vtkSMStreamingViewProxy* proxy)
    { this->StreamingView = proxy; }
  vtkGetObjectMacro(StreamingView, vtkSMStreamingViewProxy);
//BTX
protected:
  vtkSMStreamingViewHelper();
  ~vtkSMStreamingViewHelper();

  vtkSMStreamingViewProxy* StreamingView;
private:
  vtkSMStreamingViewHelper(const vtkSMStreamingViewHelper&); // Not implemented
  void operator=(const vtkSMStreamingViewHelper&); // Not implemented
//ETX
};

#endif

