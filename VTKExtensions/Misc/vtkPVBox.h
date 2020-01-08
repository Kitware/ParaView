/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBox

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVBox
 * @brief extends vtkBox to add ParaView specific API.
 *
 * vtkPVBox extends vtkBox to add ParaView specific API. We add ability to
 * provide a transform using position, scale and orientation. The transform can
 * be applied to a unit box or a explicitly specified bounds.
 */

#ifndef vtkPVBox_h
#define vtkPVBox_h

#include "vtkBox.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVBox : public vtkBox
{
public:
  static vtkPVBox* New();
  vtkTypeMacro(vtkPVBox, vtkBox);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * These bounds are used when `UseReferenceBounds` is set to true.
   * In that case, the position, rotation and scale is assumed to be relative
   * to these bounds. Otherwise, it's assumed to be absolute values i.e.
   * relative to a unit box.
   */
  void SetReferenceBounds(const double bds[6]);
  void SetReferenceBounds(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
  {
    double bds[6] = { xmin, xmax, ymin, ymax, zmin, zmax };
    this->SetReferenceBounds(bds);
  }
  vtkGetVector6Macro(ReferenceBounds, double);
  //@}

  //@{
  /**
   * Set to true to use ReferenceBounds as the basis for the transformation
   * instead of unit box.
   */
  void SetUseReferenceBounds(bool val);
  vtkGetMacro(UseReferenceBounds, bool);
  //@}

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
  ~vtkPVBox() override;
  void UpdateTransform();

  double Position[3];
  double Rotation[3];
  double Scale[3];
  double ReferenceBounds[6];
  bool UseReferenceBounds;

private:
  vtkPVBox(const vtkPVBox&) = delete;
  void operator=(const vtkPVBox&) = delete;
};

#endif
