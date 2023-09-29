// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVThreshold
 * @brief threshold filter to add support for vtkHyperTreeGrid.
 *
 * This is a subclass of vtkThreshold that allows to apply threshold filters
 * to either vtkDataSet or vtkHyperTreeGrid.
 */

#ifndef vtkPVThreshold_h
#define vtkPVThreshold_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkThreshold.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVThreshold : public vtkThreshold
{
public:
  static vtkPVThreshold* New();
  vtkTypeMacro(vtkPVThreshold, vtkThreshold);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@{
  /**
   * Property specific to `vtkHyperTreeGridThreshold`. See class doc for more info.
   *
   * default is vtkHyperTreeGridThreshold::MaskInput
   */
  vtkGetMacro(MemoryStrategy, int);
  vtkSetMacro(MemoryStrategy, int);
  ///@}

protected:
  vtkPVThreshold() = default;
  ~vtkPVThreshold() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int ThresholdUsingSuperclassInstance(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  vtkPVThreshold(const vtkPVThreshold&) = delete;
  void operator=(const vtkPVThreshold&) = delete;

  int MemoryStrategy = 0;
};

#endif
