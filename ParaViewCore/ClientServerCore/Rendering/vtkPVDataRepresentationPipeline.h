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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Expose `DataTime` timestamp. This gets changed anytime the `RequestData` is
   * called on the algorithm. This is more robust mechanism to determine if the
   * algorithm reexecuted.
   */
  vtkGetMacro(DataTime, vtkMTimeType);

protected:
  vtkPVDataRepresentationPipeline();
  ~vtkPVDataRepresentationPipeline() override;

  int ForwardUpstream(int i, int j, vtkInformation* request) override;
  int ForwardUpstream(vtkInformation* request) override;

  void ExecuteDataEnd(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;

  // Override this check to account for update extent.
  int NeedToExecuteData(
    int outputPort, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec) override;

  int ProcessRequest(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;

private:
  vtkPVDataRepresentationPipeline(const vtkPVDataRepresentationPipeline&) = delete;
  void operator=(const vtkPVDataRepresentationPipeline&) = delete;
};

#endif
