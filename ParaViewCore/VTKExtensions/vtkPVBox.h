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
// .NAME vtkPVBox - extends vtkBox to add ParaView specific API.
// .SECTION Description
// vtkPVBox extends vtkBox to add ParaView specific API.

#ifndef __vtkPVBox_h
#define __vtkPVBox_h

#include "vtkBox.h"

class VTK_EXPORT vtkPVBox : public vtkBox
{
public:
  static vtkPVBox* New();
  vtkTypeMacro(vtkPVBox, vtkBox);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set Position of the box.
  void SetPosition(double x, double y, double z)
    {
    double pos[3] = {x, y, z};
    this->SetPosition(pos);
    }
  void SetPosition(const double pos[3]);
  vtkGetVector3Macro(Position, double);

  // Description:
  // Get/Set Rotation for the box.
  void SetRotation(double x, double y, double z)
    {
    double pos[3] = {x, y, z};
    this->SetRotation(pos);
    }
  void SetRotation(const double pos[3]);
  vtkGetVector3Macro(Rotation, double);

  // Description:
  // Get/Set Scale for the box.
  void SetScale(double x, double y, double z)
    {
    double pos[3] = {x, y, z};
    this->SetScale(pos);
    }
  void SetScale(const double pos[3]);
  vtkGetVector3Macro(Scale, double);

//BTX
protected:
  vtkPVBox();
  ~vtkPVBox();
  void UpdateTransform();

  double Position[3];
  double Rotation[3];
  double Scale[3];

private:
  vtkPVBox(const vtkPVBox&); // Not implemented
  void operator=(const vtkPVBox&); // Not implemented
//ETX
};

#endif
