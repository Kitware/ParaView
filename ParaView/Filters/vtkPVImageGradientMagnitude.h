/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageGradientMagnitude.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageGradientMagnitude -

#ifndef __vtkPVImageGradientMagnitude_h
#define __vtkPVImageGradientMagnitude_h


#include "vtkImageGradientMagnitude.h"

class VTK_EXPORT vtkPVImageGradientMagnitude : public vtkImageGradientMagnitude
{
public:
  static vtkPVImageGradientMagnitude *New();
  vtkTypeRevisionMacro(vtkPVImageGradientMagnitude,vtkImageGradientMagnitude);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to compute the gradient magnitude of an arbitrary point 
  // scalar array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  

protected:
  vtkPVImageGradientMagnitude();
  ~vtkPVImageGradientMagnitude() {};

private:
  vtkPVImageGradientMagnitude(const vtkPVImageGradientMagnitude&);  // Not implemented.
  void operator=(const vtkPVImageGradientMagnitude&);  // Not implemented.
};

#endif



