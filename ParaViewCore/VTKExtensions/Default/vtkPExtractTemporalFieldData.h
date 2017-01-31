/*=========================================================================

  Program:   ParaView
  Module:    vtkPExtractTemporalFieldData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPExtractTemporalFieldData
 * @brief   parallel version of
 * vtkExtractTemporalFieldData.
 *
 * vtkPExtractTemporalFieldData adds logic to reduce the output from
 * vtkExtractTemporalFieldData so it can plotted correctly in ParaView.
 * We simply pass data on the root node since that is sufficient for the
 * use-cases we have encountered. If needed, we can reduce to root node to only
 * get the one of the non-empty leaf nodes for all ranks.
*/

#ifndef vtkPExtractTemporalFieldData_h
#define vtkPExtractTemporalFieldData_h

#include "vtkExtractTemporalFieldData.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPExtractTemporalFieldData
  : public vtkExtractTemporalFieldData
{
public:
  static vtkPExtractTemporalFieldData* New();
  vtkTypeMacro(vtkPExtractTemporalFieldData, vtkExtractTemporalFieldData);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed. By default set to
   * vtkMultiProcessController::GlobalController in the constructor.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPExtractTemporalFieldData();
  ~vtkPExtractTemporalFieldData();

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  vtkMultiProcessController* Controller;

private:
  vtkPExtractTemporalFieldData(const vtkPExtractTemporalFieldData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPExtractTemporalFieldData&) VTK_DELETE_FUNCTION;
};

#endif
