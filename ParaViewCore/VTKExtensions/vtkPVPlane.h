/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPlane - extends vtkPlane to add Offset parameter.
// .SECTION Description
// vtkPVPlane adds an offset setting to vtkPlane.
// This offset is used together with normal and origin when
// setting parameters on the represented object.

#ifndef __vtkPVPlane_h
#define __vtkPVPlane_h

#include "vtkPlane.h"

class VTK_EXPORT vtkPVPlane : public vtkPlane
{
public:
  static vtkPVPlane* New();
  vtkTypeMacro(vtkPVPlane, vtkPlane);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The origin is shifted in the direction of the normal
  // by the offset.
  vtkSetMacro(Offset, double);
  vtkGetMacro(Offset, double);

  // Description:
  // Set/Get a transformation to apply to input points before
  // executing the implicit function.
  virtual void SetTransform(vtkAbstractTransform*);
  virtual void SetTransform(const double elements[16])
    {  this->Superclass::SetTransform(elements); }

  // Description:
  // Evaluate function at position x-y-z and return value.  You should
  // generally not call this method directly, you should use
  // FunctionValue() instead.  This method must be implemented by
  // any derived class.
  virtual double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    { return this->Superclass::EvaluateFunction(x, y, z); }

  // Description:
  // Evaluate function gradient at position x-y-z and pass back vector.
  // You should generally not call this method directly, you should use
  // FunctionGradient() instead.  This method must be implemented by
  // any derived class.
  virtual void EvaluateGradient(double x[3], double g[3]);

//BTX
protected:
  vtkPVPlane();
  ~vtkPVPlane();

  double Offset;
  vtkPlane* Plane;

private:
  vtkPVPlane(const vtkPVPlane&); // Not implemented
  void operator=(const vtkPVPlane&); // Not implemented
//ETX
};

#endif
