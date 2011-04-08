/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderViewProxy - Forwards calls to the vtkPVRenderView class
//
// .SECTION Description
// This class is used to allow separation between client GUI code and server code.
// It forwards calls to the vtkPVRenderView class.
//
// .SECTION See Also
// vtkPVRenderModuleProxy

#ifndef __vtkPVRenderViewProxy_h
#define __vtkPVRenderViewProxy_h

#include "vtkObject.h"

class vtkRenderWindow;

class VTK_EXPORT vtkPVRenderViewProxy : public vtkObject
{
public:
  vtkTypeMacro(vtkPVRenderViewProxy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Forward these calls to an actual vtkPVRenderView in a sub class.
  virtual void EventuallyRender() = 0;
  virtual vtkRenderWindow* GetRenderWindow() = 0;
  virtual void Render() = 0;

  // Description:
  // Returns true if the most recent render indeed employed low-res rendering.
  virtual bool LastRenderWasInteractive()=0;

protected:
  vtkPVRenderViewProxy() {};
  ~vtkPVRenderViewProxy() {};

private:
  vtkPVRenderViewProxy(const vtkPVRenderViewProxy&); // Not implemented
  void operator=(const vtkPVRenderViewProxy&); // Not implemented
};

#endif
