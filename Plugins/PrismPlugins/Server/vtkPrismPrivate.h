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
//-----------------------------------------------------------------------------
namespace vtkPrismCommon
{

  static void reverseLog10Scale(double point[3], bool scalingEnabled[3])
    {
    //scale the point by the provided power value. 
    //remember the point is already scaled
    point[0] = scalingEnabled[0] ? point[0] : pow(10.0,point[0]);
    point[1] = scalingEnabled[1] ? point[1] : pow(10.0,point[1]);
    point[2] = scalingEnabled[2] ? point[2] : pow(10.0,point[2]);
    }
  
  static void logScale(double point[3], bool scalingEnabled[3])
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
  
  //Will scale the point as needed in place
  static void scalePoint(double point[3], bool scalingEnabled[3], const int &tableId )
    {
    switch(tableId)
      {
      case 502:
      case 503:
      case 504:
      case 505:
      case 601:
      case 602:
      case 603:
      case 604:
      case 605:
        //from the ref docs we know these values have been log10 scaled already
        //so we will unscale if needed
        reverseLog10Scale(point,scalingEnabled);
        break;
      case 301:
      default:
        //from the ref docs these point are unscaled so if requested
        //we will log scale them
        logScale(point,scalingEnabled);
        break;
      }
    }
}

#endif
