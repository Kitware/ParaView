// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPConvertSelection
 * @brief   parallel aware vtkConvertSelection subclass.
 *
 * vtkPConvertSelection is a parallel aware vtkConvertSelection subclass.
 */

#ifndef vtkPConvertSelection_h
#define vtkPConvertSelection_h

#include "vtkConvertSelection.h"
#include "vtkPVVTKExtensionsExtractionModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkPConvertSelection : public vtkConvertSelection
{
public:
  static vtkPConvertSelection* New();
  vtkTypeMacro(vtkPConvertSelection, vtkConvertSelection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPConvertSelection();
  ~vtkPConvertSelection() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;

private:
  vtkPConvertSelection(const vtkPConvertSelection&) = delete;
  void operator=(const vtkPConvertSelection&) = delete;
};

#endif // vtkPConvertSelection_h
