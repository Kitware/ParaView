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
 * @class   vtkPVCenterAxesActor
 *
 * vtkPVCenterAxesActor is an actor for the center-axes used in ParaView. It
 * merely uses vtkAxes as the poly data source.
*/

#ifndef vtkPVCenterAxesActor_h
#define vtkPVCenterAxesActor_h

#include "vtkOpenGLActor.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkAxes;
class vtkPolyDataMapper;

class VTKREMOTINGVIEWS_EXPORT vtkPVCenterAxesActor : public vtkOpenGLActor
{
public:
  static vtkPVCenterAxesActor* New();
  vtkTypeMacro(vtkPVCenterAxesActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If Symmetric is on, the the axis continue to negative values.
   */
  void SetSymmetric(int);

  /**
   * Option for computing normals.  By default they are computed.
   */
  void SetComputeNormals(int);

protected:
  vtkPVCenterAxesActor();
  ~vtkPVCenterAxesActor() override;

  vtkAxes* Axes;
  vtkPolyDataMapper* Mapper;

private:
  vtkPVCenterAxesActor(const vtkPVCenterAxesActor&) = delete;
  void operator=(const vtkPVCenterAxesActor&) = delete;
};

#endif
