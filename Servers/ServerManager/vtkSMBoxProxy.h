/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoxProxy - proxy for vtkBox
// .SECTION Description
// vtkSMBoxProxy provides a Position/Rotation/Scale interface to vtkBox.

#ifndef __vtkSMBoxProxy_h
#define __vtkSMBoxProxy_h

#include "vtkSMProxy.h"

class vtkMatrix4x4;

class VTK_EXPORT vtkSMBoxProxy : public vtkSMProxy
{
public:
  static vtkSMBoxProxy* New();
  vtkTypeMacro(vtkSMBoxProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set Position of the box.
  vtkSetVector3Macro(Position, double);
  vtkGetVector3Macro(Position, double);

  // Description:
  // Get/Set Rotation for the box.
  vtkSetVector3Macro(Rotation, double);
  vtkGetVector3Macro(Rotation, double);

  // Description:
  // Get/Set Scale for the box.
  vtkSetVector3Macro(Scale, double);
  vtkGetVector3Macro(Scale, double);
 
  // Description:
  // Push values on to the VTK objects. 
  virtual void UpdateVTKObjects()
    { return this->Superclass::UpdateVTKObjects(); }

protected:
  vtkSMBoxProxy();
  ~vtkSMBoxProxy();

  double Position[3];
  double Rotation[3];
  double Scale[3];

  // Description:
  // Push values on to the VTK objects. 
  // This computes the transform matrix from the Position/Scale/Rotation
  // values and sets that on to the vtkBox
  virtual void UpdateVTKObjects(vtkClientServerStream& stream);

  // Description:
  // Computes the transform matrix from Position/Rotation/Scale
  void GetMatrix(vtkMatrix4x4 *mat);
private:
  vtkSMBoxProxy(const vtkSMBoxProxy&); // Not implemented
  void operator=(const vtkSMBoxProxy&); // Not implemented
};

#endif

