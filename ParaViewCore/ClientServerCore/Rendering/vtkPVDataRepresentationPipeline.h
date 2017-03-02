/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentationPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDataRepresentationPipeline
 * @brief   executive for
 * vtkPVDataRepresentation.
 *
 * vtkPVDataRepresentationPipeline is an executive for vtkPVDataRepresentation.
 * In works in collaboration with the vtkPVView and vtkPVDataRepresentation to
 * ensure appropriate time/piece is requested from the upstream. This also helps
 * when caching is employed by the view.
*/

#ifndef vtkPVDataRepresentationPipeline_h
#define vtkPVDataRepresentationPipeline_h

#include "vtkCompositeDataPipeline.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVDataRepresentationPipeline
  : public vtkCompositeDataPipeline
{
public:
  static vtkPVDataRepresentationPipeline* New();
  vtkTypeMacro(vtkPVDataRepresentationPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPVDataRepresentationPipeline();
  ~vtkPVDataRepresentationPipeline();

  virtual int ForwardUpstream(int i, int j, vtkInformation* request) VTK_OVERRIDE;
  virtual int ForwardUpstream(vtkInformation* request) VTK_OVERRIDE;

  virtual void ExecuteDataEnd(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) VTK_OVERRIDE;

  // Override this check to account for update extent.
  virtual int NeedToExecuteData(int outputPort, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) VTK_OVERRIDE;

  virtual int ProcessRequest(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) VTK_OVERRIDE;

private:
  vtkPVDataRepresentationPipeline(const vtkPVDataRepresentationPipeline&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVDataRepresentationPipeline&) VTK_DELETE_FUNCTION;
};

#endif
