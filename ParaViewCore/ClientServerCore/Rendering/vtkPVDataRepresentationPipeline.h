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
 * @class vtkPVDataRepresentationPipeline
 * @brief executive for vtkPVDataRepresentation.
 *
 * vtkPVDataRepresentationPipeline is an executive for vtkPVDataRepresentation.
 * Unlike other algorithms, vtkPVDataRepresentation does not use MTime as the
 * indication that the pipeline needs to be updated, but instead uses the
 * `vtkPVDataRepresentation::NeedsUpdate` flag. This mechanism, historically
 * referenced as update-suppressor, is implemented by this class. It bypasses
 * pipeline passes unless the representation explicitly indicates it needs an
 * update.
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

  /**
   * vtkPVDataRepresentation calls this in
   * `vtkPVDataRepresentation::MarkModified` to let the executive know that it
   * should no longer suppress the pipeline passes since the pipeline has been
   * invalidated.
   */
  vtkSetMacro(NeedsUpdate, bool);
  vtkGetMacro(NeedsUpdate, bool);

protected:
  vtkPVDataRepresentationPipeline();
  ~vtkPVDataRepresentationPipeline() override;

  int ForwardUpstream(int i, int j, vtkInformation* request) override;
  int ForwardUpstream(vtkInformation* request) override;
  int ProcessRequest(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;
  void ExecuteDataEnd(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;

private:
  vtkPVDataRepresentationPipeline(const vtkPVDataRepresentationPipeline&) = delete;
  void operator=(const vtkPVDataRepresentationPipeline&) = delete;

  bool NeedsUpdate = true;
};

#endif
