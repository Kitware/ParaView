/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScenePrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPrismPrivate
//
// .SECTION Description
// Private common methods for the prism plugin
// \internal


#ifndef __vtkPrismPrivate_h
#define __vtkPrismPrivate_h

#include <math.h>
#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"

//-----------------------------------------------------------------------------
namespace vtkPrismCommon
{

inline void logScale(double point[3], bool scalingEnabled[3])
{
  //if scaling is not enabled use normal value,
  //if value is positive use log scaling,
  //if value is negative set to 0.0
  point[0] = (!scalingEnabled[0]) ? point[0] :
             (point[0] > 0.0) ? log(point[0]) : 0.0;
  point[1] = (!scalingEnabled[1]) ? point[1] :
             (point[1] > 0.0) ? log(point[1]) : 0.0;
  point[2] = (!scalingEnabled[2]) ? point[2] :
             (point[2] > 0.0) ? log(point[2]) : 0.0;
}
  
 inline bool scaleBounds(int scalingEnabled[3], const int &tableId,
   double bounds[6])
{
  (void)tableId;
  bool scaling[3] = {(scalingEnabled[0]>0),
                     (scalingEnabled[1]>0),
                     (scalingEnabled[2]>0)};

  //make two fake point that we scale
  double point1[3] = {bounds[0], bounds[2], bounds[4]};
  double point2[3] = {bounds[1], bounds[3], bounds[5]};

  //find if the bounds needs to be scaled
  logScale(point1,scaling);
  logScale(point2,scaling);

  //copy back the bounds only if they are still valid bounds
  //so if point2 < point1 don't use it
  bool valid = true;
  for(int i=0; i<3;++i)
    {
    if(point2[i]>point1[i])
      {
      bounds[(i*2)] = point1[i];
      bounds[(i*2)+1] = point2[i];
      }
    else
      {
      valid = false;
      }
    }
  return valid;
}

 inline void scaleThresholdBounds(bool scalingEnabled[3],
   double xThreshold[2], double yThreshold[2], double thresholdBounds[6])
  {
  //make two fake point that we scale
  double point1[3] = {xThreshold[0], yThreshold[0], 0.0};
  double point2[3] = {xThreshold[1], yThreshold[1], 0.0};

  //find if the threshold needs to be scaled
  logScale(point1, scalingEnabled);
  logScale(point2, scalingEnabled);

  //assign the updated value back to the threshold
  thresholdBounds[0] = point1[0];
  thresholdBounds[1] = point2[0];

  thresholdBounds[2] = point1[1];
  thresholdBounds[3] = point2[1];
  }
}
#endif
