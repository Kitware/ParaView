/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageContinuousErode3D.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageContinuousErode3D - Erosion implemented as a minimum.
//
// .SECTION Description
// This is a subclass of vtkImageContinuousErode3D that allows selection of input scalars


#ifndef __vtkPVImageContinuousErode3D_h
#define __vtkPVImageContinuousErode3D_h


#include "vtkImageContinuousErode3D.h"

class VTK_EXPORT vtkPVImageContinuousErode3D : public vtkImageContinuousErode3D
{
public:
  // Description:
  // Construct an instance of vtkPVImageContinuousErode3D filter.
  // By default zero values are eroded.
  static vtkPVImageContinuousErode3D *New();
  vtkTypeRevisionMacro(vtkPVImageContinuousErode3D,vtkImageContinuousErode3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to erode by an arbitrary point scalar array, 
  // then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  
  
protected:
  vtkPVImageContinuousErode3D();
  ~vtkPVImageContinuousErode3D();

private:
  vtkPVImageContinuousErode3D(const vtkPVImageContinuousErode3D&);  // Not implemented.
  void operator=(const vtkPVImageContinuousErode3D&);  // Not implemented.
};

#endif



