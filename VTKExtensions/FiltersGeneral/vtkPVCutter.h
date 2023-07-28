// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVCutter
 * @brief   Slice Filter
 *
 *
 * This is a subclass of vtkCutter that allows selection of input vtkHyperTreeGrid
 */

#ifndef vtkPVCutter_h
#define vtkPVCutter_h

#include "vtkCutter.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkPVPlaneCutter;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVCutter : public vtkCutter
{
public:
  vtkTypeMacro(vtkPVCutter, vtkCutter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVCutter* New();

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@
  /**
   * Only used for cutting hyper tree grids. If set to true, the dual grid is used for cutting
   */
  vtkGetMacro(Dual, bool);
  vtkSetMacro(Dual, bool);
  ///@}

protected:
  vtkPVCutter();
  ~vtkPVCutter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillInputPortInformation(int, vtkInformation* info) override;
  int FillOutputPortInformation(int, vtkInformation* info) override;

  int CutUsingSuperclassInstance(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  bool Dual = false;
  vtkNew<vtkPVPlaneCutter> PlaneCutter;

private:
  vtkPVCutter(const vtkPVCutter&) = delete;
  void operator=(const vtkPVCutter&) = delete;
};

#endif
