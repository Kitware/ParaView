/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransformProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTransformProxy - proxy for vtkTransform
// .SECTION Description
// vtkSMTransformProxy provides a Position/Rotation/Scale interface to vtkTransform. 

#ifndef __vtkSMTransformProxy_h_
#define __vtkSMTransformProxy_h_

#include "vtkSMProxy.h"
class vtkMatrix4x4;

class VTK_EXPORT vtkSMTransformProxy : public vtkSMProxy
{
public:
  static vtkSMTransformProxy* New();
  vtkTypeMacro(vtkSMTransformProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
  // Get/Set Position of the transform.
  vtkSetVector3Macro(Position, double);
  vtkGetVector3Macro(Position, double);

  // Description:
  // Get/Set Rotation for the transform.
  vtkSetVector3Macro(Rotation, double);
  vtkGetVector3Macro(Rotation, double);

  // Description:
  // Get/Set Scale for the transform.
  vtkSetVector3Macro(Scale, double);
  vtkGetVector3Macro(Scale, double);
 
  // Description:
  // Push values on to the VTK objects. 
  // This computes the transform matrix from the Position/Scale/Rotation
  // values and sets that on to the transform
  virtual void UpdateVTKObjects()
    { this->Superclass::UpdateVTKObjects(); }

protected:
  vtkSMTransformProxy();
  ~vtkSMTransformProxy();

  virtual void UpdateVTKObjects(vtkClientServerStream& stream);

  double Position[3];
  double Rotation[3];
  double Scale[3];

  // Description:
  // Computes the transform matrix from Position/Rotation/Scale
  void GetMatrix(vtkMatrix4x4 *mat);
  
private:
  vtkSMTransformProxy(const vtkSMTransformProxy&); // Not implemented
  void operator=(const vtkSMTransformProxy&); // Not implemented
};

#endif

