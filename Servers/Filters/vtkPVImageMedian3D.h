/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageMedian3D.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageMedian3D - Median Filter.
//
// .SECTION Description
// This is a subclass of vtkImageMedian3D that allows selection of input scalars

#ifndef __vtkPVImageMedian3D_h
#define __vtkPVImageMedian3D_h


#include "vtkImageMedian3D.h"

class VTK_EXPORT vtkPVImageMedian3D : public vtkImageMedian3D
{
public:
  static vtkPVImageMedian3D *New();
  vtkTypeRevisionMacro(vtkPVImageMedian3D,vtkImageMedian3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If you want the neighborhood median of an arbitrary point 
  // scalar array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  

protected:
  vtkPVImageMedian3D();
  ~vtkPVImageMedian3D();

private:
  vtkPVImageMedian3D(const vtkPVImageMedian3D&);  // Not implemented.
  void operator=(const vtkPVImageMedian3D&);  // Not implemented.
};

#endif



