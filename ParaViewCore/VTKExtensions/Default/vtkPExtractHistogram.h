/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractHistogram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPExtractHistogram - Extract histogram for parallel dataset.
// .SECTION Description
// vtkPExtractHistogram is vtkExtractHistogram subclass for parallel datasets.
// It gathers the histogram data on the root node.

#ifndef __vtkPExtractHistogram_h
#define __vtkPExtractHistogram_h

#include "vtkExtractHistogram.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkPExtractHistogram : public vtkExtractHistogram
{
public:
  static vtkPExtractHistogram* New();
  vtkTypeMacro(vtkPExtractHistogram, vtkExtractHistogram);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the multiprocess controller. If no controller is set,
  // single process is assumed.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPExtractHistogram();
  ~vtkPExtractHistogram();


  virtual int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);


  // Initialize the bin_extents using the data range for the selected 
  // array.
  virtual bool InitializeBinExtents(
    vtkInformationVector** inputVector, vtkDoubleArray* bin_extents,
    double& min, double& max);

  vtkMultiProcessController* Controller;
private:
  vtkPExtractHistogram(const vtkPExtractHistogram&); // Not implemented.
  void operator=(const vtkPExtractHistogram&); // Not implemented.
};

#endif

