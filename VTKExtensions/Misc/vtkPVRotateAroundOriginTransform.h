/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRotateAroundOriginTransform

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVRotateAroundOriginTransform
 * @brief   baseclass for all ParaView vtkTransform class.
 *
 * vtkPVRotateAroundOriginTransform extend vtkPVTransform by setting an origin of rotation
 * and overriding UpdateMatrix to rotate around this point.
*/

#ifndef vtkPVRotateAroundOriginTransform_h
#define vtkPVRotateAroundOriginTransform_h

#include "vtkPVTransform.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports
class vtkTransform;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVRotateAroundOriginTransform : public vtkPVTransform
{
public:
  static vtkPVRotateAroundOriginTransform* New();
  vtkTypeMacro(vtkPVRotateAroundOriginTransform, vtkPVTransform);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set Origin of rotation.
   */
  void SetOriginOfRotation(double xyz[3]);
  void SetOriginOfRotation(double x, double y, double z);
  vtkGetVector3Macro(OriginOfRotation, double);
  //@}

protected:
  vtkPVRotateAroundOriginTransform() = default;
  ~vtkPVRotateAroundOriginTransform() override = default;

  void UpdateMatrix() override;

  double OriginOfRotation[3] = { 0.0, 0.0, 0.0 };

private:
  vtkPVRotateAroundOriginTransform(const vtkPVRotateAroundOriginTransform&) = delete;
  void operator=(const vtkPVRotateAroundOriginTransform&) = delete;
};

#endif
