/*=========================================================================

  Program:   ParaView
  Module:    vtkPMScalarBarActorProxy

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMScalarBarActorProxy
// .SECTION Description
// vtkPMScalarBarActorProxy is the server-side class used to bind subproxy
// internally

#ifndef __vtkPMScalarBarActorProxy_h
#define __vtkPMScalarBarActorProxy_h

#include "vtkPMProxy.h"

class VTK_EXPORT vtkPMScalarBarActorProxy : public vtkPMProxy
{
public:
  static vtkPMScalarBarActorProxy* New();
  vtkTypeMacro(vtkPMScalarBarActorProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMScalarBarActorProxy();
  ~vtkPMScalarBarActorProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkPMScalarBarActorProxy(const vtkPMScalarBarActorProxy&); // Not implemented
  void operator=(const vtkPMScalarBarActorProxy&); // Not implemented
//ETX
};

#endif
