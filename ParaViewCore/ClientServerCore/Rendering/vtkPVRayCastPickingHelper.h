/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRayCastPickingHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRayCastPickingHelper - helper class that used selection and ray
// casting to find the intersection point between the user picking point
// and the concreate cell underneath.

#ifndef __vtkPVRayCastPickingHelper_h
#define __vtkPVRayCastPickingHelper_h

#include "vtkObject.h"
class vtkAlgorithm;

class VTK_EXPORT vtkPVRayCastPickingHelper : public vtkObject
{
public:
  static vtkPVRayCastPickingHelper *New();
  vtkTypeMacro(vtkPVRayCastPickingHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set input on which the selection apply
  void SetInput(vtkAlgorithm*);

  // Description:
  // Set the selection that extract the cell that intersect the ray
  void SetSelection(vtkAlgorithm*);

  // Description:
  // Set the point 1 that compose the ray
  vtkSetVector3Macro(PointA, double);
  vtkGetVector3Macro(PointA, double);

  // Description:
  // Set the point 2 that compose the ray
  vtkSetVector3Macro(PointB, double);
  vtkGetVector3Macro(PointB, double);

  // Description:
  // Compute the intersection
  void ComputeIntersection();

  // Descritpion:
  // Provide access to the resulting intersection
  vtkGetVector3Macro(Intersection, double);

protected:
  vtkPVRayCastPickingHelper();
  ~vtkPVRayCastPickingHelper();

  double Intersection[3];
  double PointA[3];
  double PointB[3];
  vtkAlgorithm* Input;
  vtkAlgorithm* Selection;

private:
  vtkPVRayCastPickingHelper(const vtkPVRayCastPickingHelper&);  // Not implemented.
  void operator=(const vtkPVRayCastPickingHelper&);  // Not implemented.
};

#endif
