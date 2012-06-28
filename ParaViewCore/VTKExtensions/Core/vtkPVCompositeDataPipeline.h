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
// .NAME vtkPVCompositeDataPipeline - executive to add support for
// vtkPVPostFilter in ParaView data pipelines.
// .SECTION Description
// vtkPVCompositeDataPipeline extends vtkCompositeDataPipeline to add ParaView
// specific extensions to the pipeline.
// \li Post Filter :- it adds support to ensure that array requests made on
//     algorithms are passed along to the input vtkPVPostFilter, if one exists.
//     vtkPVPostFilter is used to automatically extract components or generated
//     derived arrays such as magnitude array for vectors.

#ifndef __vtkPVCompositeDataPipeline_h
#define __vtkPVCompositeDataPipeline_h

#include "vtkCompositeDataPipeline.h"

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

  // Remove update/whole extent when resetting pipeline information.
  virtual void ResetPipelineInformation(int port, vtkInformation*);

private:
  vtkPVCompositeDataPipeline(const vtkPVCompositeDataPipeline&);  // Not implemented.
  void operator=(const vtkPVCompositeDataPipeline&);  // Not implemented.
};

#endif
