/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageGradientMagnitude.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageGradientMagnitude - Computes magnitude of the gradient.
//
// .SECTION Description
// This is a subclass of vtkImageGradientMagnitude that allows selection of input scalars

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



