/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayWindowProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDisplayWindowProxy -
// .SECTION Description

#ifndef __vtkSMDisplayWindowProxy_h
#define __vtkSMDisplayWindowProxy_h

#include "vtkSMProxy.h"
#include "vtkClientServerID.h" // Needed for ClientServerID

class vtkSMDisplayerProxy;

class VTK_EXPORT vtkSMDisplayWindowProxy : public vtkSMProxy
{
public:
  static vtkSMDisplayWindowProxy* New();
  vtkTypeRevisionMacro(vtkSMDisplayWindowProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  void StillRender();

  // Description:
  void InteractiveRender();

  // Description:
  void AddDisplay(vtkSMDisplayerProxy* display);

  // Description:
  virtual void UpdateVTKObjects();

  // Description:
  void WriteImage(const char* filename);

protected:
  vtkSMDisplayWindowProxy();
  ~vtkSMDisplayWindowProxy();

  vtkSMProxy* RendererProxy;
  vtkSMProxy* CameraProxy;
  vtkSMProxy* CompositeProxy;
  vtkSMProxy* WindowToImage;
  vtkSMProxy* ImageWriter;

private:
  vtkSMDisplayWindowProxy(const vtkSMDisplayWindowProxy&); // Not implemented
  void operator=(const vtkSMDisplayWindowProxy&); // Not implemented
};

#endif
