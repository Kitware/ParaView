/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMSurfaceLICRepresentationProxy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSurfaceLICRepresentationProxy
// .SECTION Description

#ifndef __vtkSMSurfaceLICRepresentationProxy_h
#define __vtkSMSurfaceLICRepresentationProxy_h

#include "vtkSMSurfaceRepresentationProxy.h"

class vtkSMSurfaceLICRepresentationProxy : public vtkSMSurfaceRepresentationProxy
{
public:
  static vtkSMSurfaceLICRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMSurfaceLICRepresentationProxy, vtkSMSurfaceRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  void  SelectInputVectors(int, int, int, int attributeMode, const char* name);

  // Description:
  // Set to 0 to disable using of LIC when interacting.
  void SetUseLICForLOD(int);

//BTX
protected:
  vtkSMSurfaceLICRepresentationProxy();
  ~vtkSMSurfaceLICRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();
private:
  vtkSMSurfaceLICRepresentationProxy(const vtkSMSurfaceLICRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSurfaceLICRepresentationProxy&); // Not implemented
//ETX
};

#endif
