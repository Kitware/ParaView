/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVThreshold - This filter extract cells using lower / upper
// threshold set and vtkPVClipDataSet filter.
//
// .SECTION Description
//
// .SECTION See Also
// vtkThreshold vtkPVClipDataSet

#ifndef __vtkPVThreshold_h
#define __vtkPVThreshold_h

#include "vtkUnstructuredGridAlgorithm.h"

// Forware declarations.
class vtkPVClipDataSet;

class VTK_EXPORT vtkPVThreshold : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPVThreshold *New();
  vtkTypeRevisionMacro(vtkPVThreshold,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds
  // (inclusive of the end values).
  void ThresholdBetween(double lower, double upper);

  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold,double);
  vtkGetMacro(LowerThreshold,double);


protected:
  vtkPVThreshold();
 ~vtkPVThreshold();

  // Usual data generation methods.
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
                             vtkInformationVector*);

  vtkGetMacro(UsingPointScalars, int);

  double LowerThreshold;
  double UpperThreshold;

  int    UsingPointScalars;

  vtkPVClipDataSet*   LowerBoundClipDS;
  vtkPVClipDataSet*   UpperBoundClipDS;

private:
  vtkPVThreshold(const vtkPVThreshold&);  // Not implemented.
  void operator=(const vtkPVThreshold&);  // Not implemented.
};

#endif // __vtkPVThreshold_h
