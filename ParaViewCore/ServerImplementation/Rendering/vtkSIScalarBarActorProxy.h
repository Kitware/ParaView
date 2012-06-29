/*=========================================================================

  Program:   ParaView
  Module:    vtkSIScalarBarActorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIScalarBarActorProxy
// .SECTION Description
// vtkSIScalarBarActorProxy is the server-side class used to bind subproxy
// internally

#ifndef __vtkSIScalarBarActorProxy_h
#define __vtkSIScalarBarActorProxy_h

#include "vtkSIProxy.h"

class VTK_EXPORT vtkSIScalarBarActorProxy : public vtkSIProxy
{
public:
  static vtkSIScalarBarActorProxy* New();
  vtkTypeMacro(vtkSIScalarBarActorProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIScalarBarActorProxy();
  ~vtkSIScalarBarActorProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkSIScalarBarActorProxy(const vtkSIScalarBarActorProxy&); // Not implemented
  void operator=(const vtkSIScalarBarActorProxy&); // Not implemented
//ETX
};

#endif
