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
/**
 * @class   vtkPVCompositeDataPipeline
 * @brief   executive to add support for
 * vtkPVPostFilter in ParaView data pipelines.
 *
 * vtkPVCompositeDataPipeline extends vtkCompositeDataPipeline to add ParaView
 * specific extensions to the pipeline.
 * \li Post Filter :- it adds support to ensure that array requests made on
 *     algorithms are passed along to the input vtkPVPostFilter, if one exists.
 *     vtkPVPostFilter is used to automatically extract components or generated
 *     derived arrays such as magnitude array for vectors.
*/

#ifndef vtkPVCompositeDataPipeline_h
#define vtkPVCompositeDataPipeline_h

#include "vtkCompositeDataPipeline.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVCompositeDataPipeline : public vtkCompositeDataPipeline
{
public:
  static vtkPVCompositeDataPipeline* New();
  vtkTypeMacro(vtkPVCompositeDataPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPVCompositeDataPipeline();
  ~vtkPVCompositeDataPipeline();

  // Copy information for the given request.
  virtual void CopyDefaultInformation(vtkInformation* request, int direction,
    vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec) VTK_OVERRIDE;

  // Remove update/whole extent when resetting pipeline information.
  virtual void ResetPipelineInformation(int port, vtkInformation*) VTK_OVERRIDE;

private:
  vtkPVCompositeDataPipeline(const vtkPVCompositeDataPipeline&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVCompositeDataPipeline&) VTK_DELETE_FUNCTION;
};

#endif
