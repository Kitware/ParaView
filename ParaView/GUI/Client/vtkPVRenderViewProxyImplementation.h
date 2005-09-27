/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewProxyImplementation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderViewProxyImplementation
// .SECTION Description
// This class is used to allow separation between client GUI code and server code.
// It forwards calls to the vtkPVRenderView class.

#ifndef __vtkPVRenderViewProxyImplementation_h
#define __vtkPVRenderViewProxyImplementation_h

#include "vtkPVRenderViewProxy.h"

class vtkPVRenderView;

class VTK_EXPORT vtkPVRenderViewProxyImplementation : public vtkPVRenderViewProxy
{
public:
  static vtkPVRenderViewProxyImplementation *New();
  vtkTypeRevisionMacro(vtkPVRenderViewProxyImplementation, vtkPVRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Forward these calls to an actual vtkPVRenderView in a sub class.
  virtual void EventuallyRender();
  virtual vtkRenderWindow* GetRenderWindow();
  virtual void Render(); 

  // Description:
  // Set/Get the PVRenderView.
  void SetPVRenderView(vtkPVRenderView *view);
  vtkGetObjectMacro(PVRenderView, vtkPVRenderView);

protected:
  vtkPVRenderViewProxyImplementation();
  ~vtkPVRenderViewProxyImplementation();

private:
  vtkPVRenderViewProxyImplementation(const vtkPVRenderViewProxyImplementation&); // Not implemented
  void operator=(const vtkPVRenderViewProxyImplementation&); // Not implemented

  vtkPVRenderView* PVRenderView;
};

#endif
