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
// .NAME vtkPVCenterAxesActor
// .SECTION Description
// vtkPVCenterAxesActor is an actor for the center-axes used in ParaView. It
// merely uses vtkAxes as the poly data source.

#ifndef __vtkPVCenterAxesActor_h
#define __vtkPVCenterAxesActor_h

#include "vtkOpenGLActor.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkAxes;
class vtkPolyDataMapper;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVCenterAxesActor : public vtkOpenGLActor
{
public:
  static vtkPVCenterAxesActor* New();
  vtkTypeMacro(vtkPVCenterAxesActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If Symetric is on, the the axis continue to negative values.
  void SetSymmetric(int);

  // Description:
  // Option for computing normals.  By default they are computed.
  void SetComputeNormals(int);

//BTX
protected:
  vtkPVCenterAxesActor();
  ~vtkPVCenterAxesActor();

  vtkAxes* Axes;
  vtkPolyDataMapper* Mapper;


private:
  vtkPVCenterAxesActor(const vtkPVCenterAxesActor&); // Not implemented
  void operator=(const vtkPVCenterAxesActor&); // Not implemented
//ETX
};

#endif
