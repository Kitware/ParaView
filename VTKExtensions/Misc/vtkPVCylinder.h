/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCylinder

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCylinder
 * @brief   extends vtkCylinder to add ParaView specific API.
 *
 * vtkPVCylinder extends vtkCylinder to add ParaView specific API.
*/

#ifndef vtkPVCylinder_h
#define vtkPVCylinder_h

#include "vtkCylinder.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVCylinder : public vtkCylinder
{
public:
  static vtkPVCylinder* New();

  vtkTypeMacro(vtkPVCylinder, vtkCylinder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the vector defining the direction of the cylinder.
   */
  void SetOrientedAxis(double x, double y, double z);
  void SetOrientedAxis(const double axis[3]);
  vtkGetVector3Macro(OrientedAxis, double);
  //@}

  // Reimplemented to update transform on change:
  void SetCenter(double x, double y, double z) override;
  void SetCenter(const double xyz[3]) override;

protected:
  vtkPVCylinder();
  ~vtkPVCylinder() override;

  void UpdateTransform();

  double OrientedAxis[3];

private:
  vtkPVCylinder(const vtkPVCylinder&) = delete;
  void operator=(const vtkPVCylinder&) = delete;
};

inline void vtkPVCylinder::SetOrientedAxis(double x, double y, double z)
{
  double axis[3] = { x, y, z };
  this->SetOrientedAxis(axis);
}

#endif
