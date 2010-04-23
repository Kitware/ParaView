/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImplicitPlaneProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMImplicitPlaneProxy - proxy for vtkPlane
// .SECTION Description
// vtkSMImplicitPlaneProxy adds an offset setting to vtkPlane.
// This offset is used together with normal and origin when
// setting parameters on the represented object.

#ifndef __vtkSMImplicitPlaneProxy_h
#define __vtkSMImplicitPlaneProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMImplicitPlaneProxy : public vtkSMProxy
{
public:
  static vtkSMImplicitPlaneProxy* New();
  vtkTypeMacro(vtkSMImplicitPlaneProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The origin is shifted in the direction of the normal
  // by the offset.
  vtkSetMacro(Offset, double);
  vtkGetMacro(Offset, double);

  // Description:
  // The origin of the plane.
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);

  // Description:
  // Push values to the VTK objects.
  virtual void UpdateVTKObjects()
    { this->Superclass::UpdateVTKObjects(); }

protected:
  vtkSMImplicitPlaneProxy();
  ~vtkSMImplicitPlaneProxy();

  virtual void UpdateVTKObjects(vtkClientServerStream& stream);
  double Origin[3];
  double Offset;

private:
  vtkSMImplicitPlaneProxy(const vtkSMImplicitPlaneProxy&); // Not implemented
  void operator=(const vtkSMImplicitPlaneProxy&); // Not implemented
};

#endif
