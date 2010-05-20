/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarActorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScalarBarActorProxy - proxy for vtkScalarBarActor.
// .SECTION Description
// This is the proxy for the vtkScalarBarActor. The only reason that this 
// proxy is not a generic proxy is to support simpler interface to
// set the positions of the actor, and to provide set up the text properties.


#ifndef __vtkSMScalarBarActorProxy_h
#define __vtkSMScalarBarActorProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMScalarBarActorProxy : public vtkSMProxy
{
public:
  static vtkSMScalarBarActorProxy* New();
  vtkTypeMacro(vtkSMScalarBarActorProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Methods to set the position and position2 coordinate 
  // on the server objects for the scalar bar.
  void SetPosition(double x, double y);
  void SetPosition2(double x, double y);

protected:
  vtkSMScalarBarActorProxy();
  ~vtkSMScalarBarActorProxy();

  virtual void CreateVTKObjects();
private:
  vtkSMScalarBarActorProxy(const vtkSMScalarBarActorProxy&); // Not implemented.
  void operator=(const vtkSMScalarBarActorProxy&); // Not implemented.
};

#endif
