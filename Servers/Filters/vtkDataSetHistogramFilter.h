/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetHistogramFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetHistogramFilter - compute a histogram of an array component
// .SECTION Description
// vtkDataSetHistogramFilter operates on a vtkDataSet to compute a histogram
// of a single component of one of its arrays. The output is a 1D vtkImageData
// of type int containing the histogram.

#ifndef __vtkDataSetHistogramFilter_h
#define __vtkDataSetHistogramFilter_h

#include "vtkImageAlgorithm.h"

class VTK_EXPORT vtkDataSetHistogramFilter : public vtkImageAlgorithm
{
public:
  static vtkDataSetHistogramFilter* New();
  vtkTypeRevisionMacro(vtkDataSetHistogramFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get - The output spacing is the dimension of each bin.
  // This ends up being the spacing of the output "image".
  // For a 1D histogram with 10 bins spanning the values 1000 to 2000,
  // this spacing should be set to 100.
  vtkSetMacro(OutputSpacing, double);
  vtkGetMacro(OutputSpacing, double);

  // Description:
  // Set/Get - The output origin is the location of bin (0, 0, 0).
  // Note that if the output extent does not include the value (0,0,0),
  // then this origin bin will not actually be in the output.
  // For a 1D histogram with 10 bins spanning the values 1000 to 2000,
  // this origin should be set to 1000.
  vtkSetMacro(OutputOrigin, double);
  vtkGetMacro(OutputOrigin, double);

  // Description:
  // Set/Get - The output extent sets the number/extent of the bins.
  // For a 1D histogram with 10 bins spanning the values 1000 to 2000,
  // this extent should be set to 0, 9.
  // The extent specifies inclusive min/max values.
  // This implies the the top extent should be set to the number of bins - 1.
  vtkSetVector2Macro(OutputExtent, int);
  vtkGetVector2Macro(OutputExtent, int);

  // Description:
  // Set/get which component of the specified input array to process.
  // Defaults to 0.
  vtkSetMacro(Component, int);
  vtkGetMacro(Component, int);

protected:
  vtkDataSetHistogramFilter();
  ~vtkDataSetHistogramFilter();

  double OutputSpacing;
  double OutputOrigin;
  int OutputExtent[2];
  int Component;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkDataSetHistogramFilter(const vtkDataSetHistogramFilter&); // Not implemented.
  void operator=(const vtkDataSetHistogramFilter&); // Not implemented.
};

#endif
