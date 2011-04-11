/*=========================================================================

  Program:   ParaView
  Module:    vtkCameraInterpolator2.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCameraInterpolator2
// .SECTION Description
//

#ifndef __vtkCameraInterpolator2_h
#define __vtkCameraInterpolator2_h

#include "vtkObject.h"

class vtkPoints;
class vtkParametricSpline;
class vtkCamera;

class VTK_EXPORT vtkCameraInterpolator2 : public vtkObject
{
public:
  static vtkCameraInterpolator2* New();
  vtkTypeMacro(vtkCameraInterpolator2, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Add points on the path. For a fixed location, simply add 1 point.
  void AddPositionPathPoint(double x, double y, double z);
  void ClearPositionPath();

  // Add points on the path. For a fixed location, simply add 1 point.
  void AddFocalPathPoint(double x, double y, double z);
  void ClearFocalPath();

  //BTX
  enum Modes 
    {
    PATH,
    FIXED_DIRECTION,
    LOOK_AHEAD,
    ORTHOGONAL
    };
  //ETX


  // Description:
  // Mode for managing the focal point.
  // At least one of the two modes FocalPointMode or PositionMode must be PATH
  // for the animation to have any effect.
  vtkSetClampMacro(FocalPointMode, int, PATH, ORTHOGONAL);
  vtkGetMacro(FocalPointMode, int);

  // Description:
  // Mode for managing the camera position.
  // At least one of the two modes FocalPointMode or PositionMode must be PATH
  // for the animation to have any effect.
  vtkSetClampMacro(PositionMode, int, PATH, ORTHOGONAL);
  vtkGetMacro(PositionMode, int);

  //BTX
  enum PathInterpolationModes
    {
    LINEAR,
    SPLINE
    };
  //ETX

  // Support for interpolation modes hasn't been implemented yet.
  vtkSetClampMacro(PositionPathInterpolationMode, int, LINEAR, SPLINE);
  vtkGetMacro(PositionPathInterpolationMode, int);

  // Support for interpolation modes hasn't been implemented yet.
  vtkSetClampMacro(FocalPathInterpolationMode, int, LINEAR, SPLINE);
  vtkGetMacro(FocalPathInterpolationMode, int);

  // Description:
  // When set, the FocalPointPath is treated as a closed path.
  vtkSetMacro(ClosedFocalPath, bool);
  vtkGetMacro(ClosedFocalPath, bool);
  vtkBooleanMacro(ClosedFocalPath, bool);

  // Description:
  // When set, the PositionPath is treated as a closed path.
  vtkSetMacro(ClosedPositionPath, bool);
  vtkGetMacro(ClosedPositionPath, bool);
  vtkBooleanMacro(ClosedPositionPath, bool);

  // Description:
  // \c u has to be in the range [0, 1].
  void InterpolateCamera(double u, vtkCamera*);

//BTX
protected:
  vtkCameraInterpolator2();
  ~vtkCameraInterpolator2();

  void Evaluate(double u, vtkParametricSpline* spline, double tuple[3]);

  int PositionMode;
  int FocalPointMode;
  int PositionPathInterpolationMode;
  int FocalPathInterpolationMode;
  bool ClosedPositionPath;
  bool ClosedFocalPath;

  vtkPoints* FocalPathPoints;
  vtkPoints* PositionPathPoints;

  vtkParametricSpline* FocalSpline;
  vtkParametricSpline* PositionSpline;

private:
  vtkCameraInterpolator2(const vtkCameraInterpolator2&); // Not implemented
  void operator=(const vtkCameraInterpolator2&); // Not implemented

//ETX
};

#endif

