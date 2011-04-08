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
// .NAME vtkPVFrustumActor
// .SECTION Description
// vtkPVFrustumActor is an actor that renders a frustum. Used in ParaView to
// show the frustum used for frustum selection extraction.

#ifndef __vtkPVFrustumActor_h
#define __vtkPVFrustumActor_h

#include "vtkOpenGLActor.h"

class vtkOutlineSource;
class vtkPolyDataMapper;

class VTK_EXPORT vtkPVFrustumActor : public vtkOpenGLActor
{
public:
  static vtkPVFrustumActor* New();
  vtkTypeMacro(vtkPVFrustumActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the frustum.
  void SetFrustum(double corners[24]);

  // Description:
  // Convenience method to set the color.
  void SetColor(double r, double g, double b);
  void SetLineWidth(double r);

//BTX
protected:
  vtkPVFrustumActor();
  ~vtkPVFrustumActor();

  vtkOutlineSource* Outline;
  vtkPolyDataMapper* Mapper;

private:
  vtkPVFrustumActor(const vtkPVFrustumActor&); // Not implemented
  void operator=(const vtkPVFrustumActor&); // Not implemented
//ETX
};

#endif
