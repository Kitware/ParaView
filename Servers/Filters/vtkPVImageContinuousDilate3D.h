/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageContinuousDilate3D.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageContinuousDilate3D - Dilate implemented as a maximum.
//
// .SECTION Description
// This is a subclass of vtkImageContinuousDilate3D that allows selection of input scalars

#ifndef __vtkPVImageContinuousDilate3D_h
#define __vtkPVImageContinuousDilate3D_h


#include "vtkImageContinuousDilate3D.h"

class VTK_EXPORT vtkPVImageContinuousDilate3D : public vtkImageContinuousDilate3D
{
public:

  // Description:
  static vtkPVImageContinuousDilate3D *New();
  vtkTypeRevisionMacro(vtkPVImageContinuousDilate3D,vtkImageContinuousDilate3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to dilate by an arbitrary point scalar array, 
  // then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  
  
protected:
  vtkPVImageContinuousDilate3D();
  ~vtkPVImageContinuousDilate3D();

private:
  vtkPVImageContinuousDilate3D(const vtkPVImageContinuousDilate3D&);  // Not implemented.
  void operator=(const vtkPVImageContinuousDilate3D&);  // Not implemented.
};

#endif



