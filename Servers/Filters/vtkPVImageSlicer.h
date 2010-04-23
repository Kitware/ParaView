/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageSlicer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageSlicer - helper filter used by paraview to create a single
// slice out of a 3D image data. Internally uses vtkExtractVOI.
// .SECTION Description
// vtkPVImageSlicer is a image filter that extracts a slice out of a 3D image
// data. If the input image is a 2D or 1D image, it is simply passed through.
// Otherwise the user identified slice is passed through. It is possible for
// this filter to output an empty image data, if the requested slice is not
// available in the input.

#ifndef __vtkPVImageSlicer_h
#define __vtkPVImageSlicer_h

#include "vtkImageAlgorithm.h"
#include "vtkStructuredData.h" // for VTK_*_PLANE

class VTK_EXPORT vtkPVImageSlicer : public vtkImageAlgorithm
{
public:
  static vtkPVImageSlicer* New();
  vtkTypeMacro(vtkPVImageSlicer, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get set the slice number to extract. 
  vtkSetMacro(Slice, unsigned int);
  vtkGetMacro(Slice, unsigned int);

  //BTX
  enum
    {
    XY_PLANE = VTK_XY_PLANE,
    XZ_PLANE = VTK_XZ_PLANE,
    YZ_PLANE = VTK_YZ_PLANE
    };
  //ETX

  // Description:
  // Get/Set the direction in which to slice a 3D input data.
  vtkSetClampMacro(SliceMode, int, VTK_XY_PLANE, VTK_XZ_PLANE);
  vtkGetMacro(SliceMode, int);

//BTX
protected:
  vtkPVImageSlicer();
  ~vtkPVImageSlicer();

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  unsigned int Slice;
  int SliceMode;
private:
  vtkPVImageSlicer(const vtkPVImageSlicer&); // Not implemented
  void operator=(const vtkPVImageSlicer&); // Not implemented
//ETX
};

#endif

