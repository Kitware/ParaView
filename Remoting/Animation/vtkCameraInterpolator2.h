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
/**
 * @class   vtkCameraInterpolator2
 *
 *
*/

#ifndef vtkCameraInterpolator2_h
#define vtkCameraInterpolator2_h

#include "vtkObject.h"
#include "vtkRemotingAnimationModule.h" // needed for export macro

class vtkPoints;
class vtkParametricSpline;
class vtkCamera;

class VTKREMOTINGANIMATION_EXPORT vtkCameraInterpolator2 : public vtkObject
{
public:
  static vtkCameraInterpolator2* New();
  vtkTypeMacro(vtkCameraInterpolator2, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Add points on the path. For a fixed location, simply add 1 point.
  void AddPositionPathPoint(double x, double y, double z);
  void ClearPositionPath();

  // Add points on the path. For a fixed location, simply add 1 point.
  void AddFocalPathPoint(double x, double y, double z);
  void ClearFocalPath();

  enum Modes
  {
    PATH,
    FIXED_DIRECTION,
    LOOK_AHEAD,
    ORTHOGONAL
  };

  //@{
  /**
   * Mode for managing the focal point.
   * At least one of the two modes FocalPointMode or PositionMode must be PATH
   * for the animation to have any effect.
   */
  vtkSetClampMacro(FocalPointMode, int, PATH, ORTHOGONAL);
  vtkGetMacro(FocalPointMode, int);
  //@}

  //@{
  /**
   * Mode for managing the camera position.
   * At least one of the two modes FocalPointMode or PositionMode must be PATH
   * for the animation to have any effect.
   */
  vtkSetClampMacro(PositionMode, int, PATH, ORTHOGONAL);
  vtkGetMacro(PositionMode, int);
  //@}

  enum PathInterpolationModes
  {
    LINEAR,
    SPLINE
  };

  // Support for interpolation modes hasn't been implemented yet.
  vtkSetClampMacro(PositionPathInterpolationMode, int, LINEAR, SPLINE);
  vtkGetMacro(PositionPathInterpolationMode, int);

  // Support for interpolation modes hasn't been implemented yet.
  vtkSetClampMacro(FocalPathInterpolationMode, int, LINEAR, SPLINE);
  vtkGetMacro(FocalPathInterpolationMode, int);

  //@{
  /**
   * When set, the FocalPointPath is treated as a closed path.
   */
  vtkSetMacro(ClosedFocalPath, bool);
  vtkGetMacro(ClosedFocalPath, bool);
  vtkBooleanMacro(ClosedFocalPath, bool);
  //@}

  //@{
  /**
   * When set, the PositionPath is treated as a closed path.
   */
  vtkSetMacro(ClosedPositionPath, bool);
  vtkGetMacro(ClosedPositionPath, bool);
  vtkBooleanMacro(ClosedPositionPath, bool);
  //@}

  /**
   * \c u has to be in the range [0, 1].
   */
  void InterpolateCamera(double u, vtkCamera*);

protected:
  vtkCameraInterpolator2();
  ~vtkCameraInterpolator2() override;

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
  vtkCameraInterpolator2(const vtkCameraInterpolator2&) = delete;
  void operator=(const vtkCameraInterpolator2&) = delete;
};

#endif
