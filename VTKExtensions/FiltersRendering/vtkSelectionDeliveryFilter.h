// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSelectionDeliveryFilter
 *
 * vtkSelectionDeliveryFilter is a filter that can deliver vtkSelection from
 * data-server nodes to the client. This should not be instantiated on the
 * pure-render-server nodes to avoid odd side effects (We can fix this later if
 * the need arises).
 */

#ifndef vtkSelectionDeliveryFilter_h
#define vtkSelectionDeliveryFilter_h

#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkSelectionAlgorithm.h"

class vtkClientServerMoveData;
class vtkReductionFilter;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkSelectionDeliveryFilter
  : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionDeliveryFilter* New();
  vtkTypeMacro(vtkSelectionDeliveryFilter, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSelectionDeliveryFilter();
  ~vtkSelectionDeliveryFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;

private:
  vtkSelectionDeliveryFilter(const vtkSelectionDeliveryFilter&) = delete;
  void operator=(const vtkSelectionDeliveryFilter&) = delete;
};

#endif
