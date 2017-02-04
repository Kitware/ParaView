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
 * @class   vtkPVBox
 * @brief   extends vtkBox to add ParaView specific API.
 *
 * vtkPVBox extends vtkBox to add ParaView specific API.
*/

#ifndef vtkPVBox_h
#define vtkPVBox_h

#include "vtkBox.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVBox : public vtkBox
{
public:
  static vtkPVBox* New();
  vtkTypeMacro(vtkPVBox, vtkBox);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set Position of the box.
   */
  void SetPosition(double x, double y, double z)
  {
    double pos[3] = { x, y, z };
    this->SetPosition(pos);
  }
  void SetPosition(const double pos[3]);
  vtkGetVector3Macro(Position, double);
  //@}

  //@{
  /**
   * Get/Set Rotation for the box.
   */
  void SetRotation(double x, double y, double z)
  {
    double pos[3] = { x, y, z };
    this->SetRotation(pos);
  }
  void SetRotation(const double pos[3]);
  vtkGetVector3Macro(Rotation, double);
  //@}

  //@{
  /**
   * Get/Set Scale for the box.
   */
  void SetScale(double x, double y, double z)
  {
    double pos[3] = { x, y, z };
    this->SetScale(pos);
  }
  void SetScale(const double pos[3]);
  vtkGetVector3Macro(Scale, double);
  //@}

protected:
  vtkPVBox();
  ~vtkPVBox();
  void UpdateTransform();

  double Position[3];
  double Rotation[3];
  double Scale[3];

private:
  vtkPVBox(const vtkPVBox&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVBox&) VTK_DELETE_FUNCTION;
};

#endif
