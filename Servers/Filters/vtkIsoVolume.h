/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIsoVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIsoVolume - This filter extract cells using lower / upper
// threshold set and vtkPVClipDataSet filter.
//
// .SECTION Description
//
// .SECTION See Also
// vtkThreshold vtkPVClipDataSet

#ifndef __vtkIsoVolume_h
#define __vtkIsoVolume_h

#include "vtkUnstructuredGridAlgorithm.h"

// Forware declarations.
class vtkPVClipDataSet;
class vtkThreshold;

class VTK_EXPORT vtkIsoVolume : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkIsoVolume *New();
  vtkTypeMacro(vtkIsoVolume,vtkUnstructuredGridAlgorithm);
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
  vtkIsoVolume();
 ~vtkIsoVolume();

  // Usual data generation methods.
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
                             vtkInformationVector*);

  vtkGetMacro(UsingPointScalars, int);

  double LowerThreshold;
  double UpperThreshold;

  bool   UsingPointScalars;

  vtkPVClipDataSet*   LowerBoundClipDS;
  vtkPVClipDataSet*   UpperBoundClipDS;

  vtkThreshold*       Threshold;

private:
  vtkIsoVolume(const vtkIsoVolume&);  // Not implemented.
  void operator=(const vtkIsoVolume&);  // Not implemented.
};

#endif // __vtkIsoVolume_h
