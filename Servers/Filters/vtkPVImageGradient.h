/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageGradient.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageGradient - Computes the gradient vector.
//
// .SECTION Description
// This is a subclass of vtkImageGradient that allows selection of input scalars

#ifndef __vtkPVImageGradient_h
#define __vtkPVImageGradient_h


#include "vtkImageGradient.h"

class VTK_EXPORT vtkPVImageGradient : public vtkImageGradient
{
public:
  static vtkPVImageGradient *New();
  vtkTypeRevisionMacro(vtkPVImageGradient,vtkImageGradient);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to compute the gradient of an arbitrary point scalar array, 
  // then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  

protected:
  vtkPVImageGradient();
  ~vtkPVImageGradient() {};

private:
  vtkPVImageGradient(const vtkPVImageGradient&);  // Not implemented.
  void operator=(const vtkPVImageGradient&);  // Not implemented.
};

#endif



