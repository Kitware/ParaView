/*=========================================================================

  Module:    vtkKWMath.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMath - performs more common math operations
// .SECTION Description
// vtkKWMath is provides methods to perform more common math operations.

#ifndef __vtkKWMath_h
#define __vtkKWMath_h

#include "vtkObject.h"

class vtkColorTransferFunction;
class vtkDataArray;
class vtkPiecewiseFunction;

class VTK_EXPORT vtkKWMath : public vtkObject
{
public:
  static vtkKWMath *New();
  vtkTypeRevisionMacro(vtkKWMath,vtkObject);

  // Description:
  // Rounds a float to the nearest integer.
  //BTX
  static float Round(float f) {
    return (f >= 0 ? (float)floor((double)f + 0.5) : ceil((double)f - 0.5)); }
  static double Round(double f) {
    return (f >= 0 ? floor(f + 0.5) : ceil(f - 0.5)); }
  //ETX

  // Description:
  // Get the data's scalar range given a component
  // Return 1 on success, 0 otherwise.
  static int GetScalarRange(vtkDataArray *array, int comp, double range[2]);
 
  // Description:
  // Get the data's scalar range given a component. This range is adjusted
  // for unsigned char and short to the whole data type range (or 4095 if
  // the real range is within that limit).
  // Return 1 on success, 0 otherwise.
  static int GetAdjustedScalarRange(
    vtkDataArray *array, int comp, double range[2]);
 
  // Description:
  // Get the scalar type that will be able to store a given range of data 
  // once it has been scaled and shifted. If any of those parameters is not
  // an integer number, the search will default to float types (float, double)
  // Return -1 on error or no scalar type found.
  static int GetScalarTypeFittingRange(
    double range_min, double range_max, 
    double scale = 1.0, double shift = 0.0);
 
  // Description:
  // Remove points out of the adjusted range of the array for a given 
  // component (see GetAdjustedScalarRange), and make sure there is a point
  // at each end of that range.
  // Return 1 on success, 0 otherwise.
  static int FixTransferFunctionPointsOutOfRange(
    vtkPiecewiseFunction *func, double range[2]);
  static int FixTransferFunctionPointsOutOfRange(
    vtkColorTransferFunction *func, double range[2]);
 
  // Description:
  // Return true if first extent is within second extent
  static int ExtentIsWithinOtherExtent(int extent1[6], int extent2[6]);

  // Description:
  static void LabToXYZ(double lab[3], double xyz[3]);
  static void XYZToRGB(double xyz[3], double rgb[3]);

protected:
  vtkKWMath() {};
  ~vtkKWMath() {};
  
private:
  vtkKWMath(const vtkKWMath&);  // Not implemented.
  void operator=(const vtkKWMath&);  // Not implemented.
};

#endif
