/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImplicitPlaneRepresentation - extends vtkImplicitPlaneRepresentation
// .SECTION Description
// vtkPVImplicitPlaneRepresentation extends vtkImplicitPlaneRepresentation to
// add ParaView proper initialisation values

#ifndef __vtkPVImplicitPlaneRepresentation_h
#define __vtkPVImplicitPlaneRepresentation_h

#include "vtkImplicitPlaneRepresentation.h"

class vtkTransform;
class VTK_EXPORT vtkPVImplicitPlaneRepresentation : public vtkImplicitPlaneRepresentation
{
public:
  static vtkPVImplicitPlaneRepresentation* New();
  vtkTypeMacro(vtkPVImplicitPlaneRepresentation, vtkImplicitPlaneRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Set the transform that this plane rep is going to be scaled by
  void SetTransform(vtkTransform *transform);
  void ClearTransform();

  void SetTransformedOrigin(double x, double y, double z);
  void SetTransformedNormal(double x, double y, double z);
  
  double* GetTransformedOrigin();
  double* GetTransformedNormal();
  
  void PlaceTransformedWidget(double bounds[6]);
  void UpdateTransformLocation();

  void Reset();
  
//BTX
protected:
  vtkPVImplicitPlaneRepresentation();
  ~vtkPVImplicitPlaneRepresentation();

  vtkTransform* Transform;
  vtkTransform* InverseTransform;

  class vtkPVInternal;
  vtkPVInternal *Internal;

private:
  vtkPVImplicitPlaneRepresentation(const vtkPVImplicitPlaneRepresentation&); // Not implemented
  void operator=(const vtkPVImplicitPlaneRepresentation&); // Not implemented
//ETX
};

#endif
