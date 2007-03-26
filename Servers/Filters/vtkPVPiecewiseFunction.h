/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPiecewiseFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPiecewiseFunction - Extension to vtkPiecewiseFunction
// that provides API to adjust control points based on scalar range.
// .SECTION Description
// vtkPVPiecewiseFunction is an extension to vtkPiecewiseFunction
// which allows for the control points to be changed based on the scalar 
// range. When SetRange() is called and ScalePointsWithRange it true,
// the the control points are scaled to fit the new range. If ScalePointsWithRange
// is false, SetRange() has no effect.
// .SECTION See Also
// vtkPVLookupTable

#ifndef __vtkPVPiecewiseFunction_h
#define __vtkPVPiecewiseFunction_h

#include "vtkPiecewiseFunction.h"

class VTK_EXPORT vtkPVPiecewiseFunction : public vtkPiecewiseFunction
{
public:
  static vtkPVPiecewiseFunction* New();
  vtkTypeRevisionMacro(vtkPVPiecewiseFunction, vtkPiecewiseFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sets/Gets the range of scalars which will be mapped.
  // If ScalePointsWithRange is off, this has no effect on the
  // lookup table. Is ScalePointsWithRange is 1, then all control 
  // points in the vtkPiecewiseFunction will be scaled to fit the new range.
  virtual void SetRange(double min, double max);

  // Description:
  // When set to 1, SetRange() results in scaling of the points 
  // in the piecewise function to fit the new range.
  // By default, set to 1.
  vtkSetMacro(ScalePointsWithRange, int);
  vtkGetMacro(ScalePointsWithRange, int);
  vtkBooleanMacro(ScalePointsWithRange, int);

protected:
  vtkPVPiecewiseFunction();
  ~vtkPVPiecewiseFunction();

  int ScalePointsWithRange;
private:
  vtkPVPiecewiseFunction(const vtkPVPiecewiseFunction&); // Not implemented.
  void operator=(const vtkPVPiecewiseFunction&); // Not implemented.
};

#endif

