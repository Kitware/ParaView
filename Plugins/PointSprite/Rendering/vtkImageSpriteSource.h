/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpriteSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkImageSpriteSource - Create an image with Gaussian pixel values and alpha.
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// vtkImageSpriteSource produces images with luminance values determined
// by a Gaussian. The alpha values can be produced to either be a gaussian
// or create a circular mask.


#ifndef __vtkImageSpriteSource_h
#define __vtkImageSpriteSource_h

#include "vtkImageAlgorithm.h"

class VTK_EXPORT vtkImageSpriteSource : public vtkImageAlgorithm
{
public:
  static vtkImageSpriteSource *New();
  vtkTypeMacro(vtkImageSpriteSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);

  // Description:
  // Set/Get the Maximum value of the gaussian
  vtkSetMacro(Maximum, unsigned char);
  vtkGetMacro(Maximum, unsigned char);

  // Description:
  // Set/Get the standard deviation of the gaussian (image coordinates are between 0 and 1)
  vtkSetMacro(StandardDeviation, double);
  vtkGetMacro(StandardDeviation, double);

  // Description:
  // Set/Get the alpha method :
  // 0.NONE will create no alpha component.
  // 1.PROPORTIONAL will create an alpha component identical to the luminance
  // 2.CLAMP will set the alpha component to 0 or 1 depending if the luminance
  // is inferior or superior to the given threhold.
  vtkSetMacro(AlphaMethod, int);
  vtkGetMacro(AlphaMethod, int);
  //BTX
  enum {NONE = 0, PROPORTIONAL=1, CLAMP=2};
  //ETX

  // Description:
  // Set/Get the alpha threshold used if the AlphaMethod is CLAMP.
  vtkSetMacro(AlphaThreshold, unsigned char);
  vtkGetMacro(AlphaThreshold, unsigned char);

protected:
  vtkImageSpriteSource();
  ~vtkImageSpriteSource() {};

  double StandardDeviation;
  int WholeExtent[6];
  unsigned char Maximum;
  int AlphaMethod;
  unsigned char AlphaThreshold;

  virtual int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkImageSpriteSource(const vtkImageSpriteSource&);  // Not implemented.
  void operator=(const vtkImageSpriteSource&);  // Not implemented.
};


#endif
