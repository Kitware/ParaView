/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayerProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDisplayerProxy -
// .SECTION Description

#ifndef __vtkSMDisplayerProxy_h
#define __vtkSMDisplayerProxy_h

#include "vtkSMSourceProxy.h"

class vtkSMDisplayerProxyInternals;
class vtkSMPart;

class VTK_EXPORT vtkSMDisplayerProxy : public vtkSMSourceProxy
{
public:
  static vtkSMDisplayerProxy* New();
  vtkTypeRevisionMacro(vtkSMDisplayerProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  virtual void UpdateVTKObjects();

  // Description:
  void SetScalarVisibility(int vis);

  // Description:
  void SetRepresentation(int repr);

protected:
  vtkSMDisplayerProxy();
  ~vtkSMDisplayerProxy();

//BTX
  friend class vtkSMDisplayWindowProxy;
//ETX

  // Description:
  vtkGetObjectMacro(ActorProxy, vtkSMProxy);

  vtkSMProxy* MapperProxy;
  vtkSMProxy* ActorProxy;
  vtkSMProxy* PropertyProxy;

  void DrawWireframe();
  void DrawPoints();
  void DrawSurface();

private:
  vtkSMDisplayerProxy(const vtkSMDisplayerProxy&); // Not implemented
  void operator=(const vtkSMDisplayerProxy&); // Not implemented
};

#endif
