// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVExtractComponent
 * @brief   Extract a component of an attribute.
 *
 * vtkPVExtractComponent Extract a component of an attribute.
 */

#ifndef vtkPVExtractComponent_h
#define vtkPVExtractComponent_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkDataSet;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVExtractComponent : public vtkDataSetAlgorithm
{
public:
  static vtkPVExtractComponent* New();
  vtkTypeMacro(vtkPVExtractComponent, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(InputArrayComponent, int);
  vtkGetMacro(InputArrayComponent, int);

  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);

protected:
  vtkPVExtractComponent();
  ~vtkPVExtractComponent() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int InputArrayComponent;
  char* OutputArrayName;

private:
  vtkPVExtractComponent(const vtkPVExtractComponent&) = delete;
  void operator=(const vtkPVExtractComponent&) = delete;
};

#endif
