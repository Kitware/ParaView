/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCompositeDataPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeDataPipeline - Executive supporting composite datasets.
// .SECTION Description
// vtkPVCompositeDataPipeline is an executive that supports the processing of
// composite dataset. It supports algorithms that are aware of composite
// dataset as well as those that are not. Type checking is performed at run
// time. Algorithms that are not composite dataset-aware have to support
// all dataset types contained in the composite dataset. The pipeline
// execution can be summarized as follows:
//
// * REQUEST_INFORMATION: The producers have to provide information about
// the contents of the composite dataset in this pass.
// Sources that can produce more than one piece (note that a piece is
// different than a block; each piece consistes of 0 or more blocks) should
// set MAXIMUM_NUMBER_OF_PIECES to -1.
//
// * REQUEST_DATA: This is where the algorithms execute. If the
// vtkPVCompositeDataPipeline is assigned to a simple filter,
// it will invoke the  vtkStreamingDemandDrivenPipeline passes in a loop,
// passing a different block each time and will collect the results in a
// composite dataset.
// .SECTION See also
//  vtkCompositeDataSet

#ifndef __vtkPVCompositeDataPipeline_h
#define __vtkPVCompositeDataPipeline_h

#include "vtkCompositeDataPipeline.h"

class vtkCompositeDataSet;
class vtkInformationDoubleKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationDataObjectKey;
class vtkInformationIntegerKey;

class VTK_EXPORT vtkPVCompositeDataPipeline : public vtkCompositeDataPipeline
{
public:
  static vtkPVCompositeDataPipeline* New();
  vtkTypeMacro(vtkPVCompositeDataPipeline,vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVCompositeDataPipeline();
  ~vtkPVCompositeDataPipeline();

  // Copy information for the given request.
  virtual void CopyDefaultInformation(vtkInformation* request, int direction,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec);


private:
  vtkPVCompositeDataPipeline(const vtkPVCompositeDataPipeline&);  // Not implemented.
  void operator=(const vtkPVCompositeDataPipeline&);  // Not implemented.
};

#endif
