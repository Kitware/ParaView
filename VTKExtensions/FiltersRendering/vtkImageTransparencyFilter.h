/*=========================================================================

  Program:   ParaView
  Module:    vtkImageTransparencyFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageTransparencyFilter
 * @brief   Filter that creates a transparent image from two input images.
 *
 * This filter takes two inputs, one of which is a scene rendered with a white
 * background and the other is the same scene rendered with a black background.
 * It looks at co-located pixels from each input image. If they are not the same
 * color, the pixel must have been from a transparent object. At such pixel
 * locations in the inputs, the transparency value in the output image is
 * derived from the difference of the HSV colors from the white- and black-
 * background images.
 *
 * The first input must be the white-background image and the second input must
 * be the black-background image.
 */

#ifndef vtkImageTransparencyFilter_h
#define vtkImageTransparencyFilter_h

#include "vtkImageAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkImageTransparencyFilter
  : public vtkImageAlgorithm
{
public:
  static vtkImageTransparencyFilter* New();
  vtkTypeMacro(vtkImageTransparencyFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  // Override to set two inputs
  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkImageTransparencyFilter();
  ~vtkImageTransparencyFilter();

private:
  vtkImageTransparencyFilter(const vtkImageTransparencyFilter&) = delete;
  void operator=(const vtkImageTransparencyFilter&) = delete;
};

#endif
