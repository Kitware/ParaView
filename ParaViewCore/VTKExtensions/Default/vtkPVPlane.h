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
/**
 * @class   vtkPVPlane
 * @brief   extends vtkPlane to add Offset parameter.
 *
 * vtkPVPlane adds an offset setting to vtkPlane.
 * This offset is used together with normal and origin when
 * setting parameters on the represented object.
*/

#ifndef vtkPVPlane_h
#define vtkPVPlane_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPlane.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVPlane : public vtkPlane
{
public:
  static vtkPVPlane* New();
  vtkTypeMacro(vtkPVPlane, vtkPlane);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The origin is shifted in the direction of the normal
   * by the offset.
   */
  vtkSetMacro(Offset, double);
  vtkGetMacro(Offset, double);
  //@}

  /**
   * Set/Get a transformation to apply to input points before
   * executing the implicit function.
   */
  void SetTransform(vtkAbstractTransform*) override;
  void SetTransform(const double elements[16]) override
  {
    this->Superclass::SetTransform(elements);
  }

  /**
   * Evaluate function at position x-y-z and return value.  You should
   * generally not call this method directly, you should use
   * FunctionValue() instead.  This method must be implemented by
   * any derived class.
   */
  using Superclass::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;

  /**
   * Evaluate function gradient at position x-y-z and pass back vector.
   * You should generally not call this method directly, you should use
   * FunctionGradient() instead.  This method must be implemented by
   * any derived class.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

protected:
  vtkPVPlane();
  ~vtkPVPlane() override;

  double Offset;
  vtkPlane* Plane;

private:
  vtkPVPlane(const vtkPVPlane&) = delete;
  void operator=(const vtkPVPlane&) = delete;
};

#endif
