// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2025 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkExtractDescriptiveStatistics
 * @brief   Extract tables from descriptive-statistics models.
 *
 * This class accepts a single statistical model or a partitioned dataset collection
 * of statistical models as input and produces a partitioned dataset collection
 * holding model tables as output.
 *
 * Once model tables are extracted, they can be examined in ParaView's
 * spreadsheet view but cannot be used to evaluate data any longer.
 */

#ifndef vtkExtractDescriptiveStatistics_h
#define vtkExtractDescriptiveStatistics_h

#include "vtkMultiProcessController.h"                 // For ivar
#include "vtkPVVTKExtensionsFiltersStatisticsModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class vtkDataObjectCollection;
class vtkStatisticalModel;
class vtkStdString;
class vtkStringArray;
class vtkVariant;
class vtkVariantArray;
class vtkDoubleArray;
class vtkExtractDescriptiveStatisticsPrivate;

class VTKPVVTKEXTENSIONSFILTERSSTATISTICS_EXPORT vtkExtractDescriptiveStatistics
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  vtkTypeMacro(vtkExtractDescriptiveStatistics, vtkPartitionedDataSetCollectionAlgorithm);
  static vtkExtractDescriptiveStatistics* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the controller used to handle distributed data.
   *
   * For this filter, only rank 0 will have any output since the information is
   * intended only for display purposes in the spreadsheet view.
   */
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkExtractDescriptiveStatistics();
  ~vtkExtractDescriptiveStatistics() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExtractDescriptiveStatistics(const vtkExtractDescriptiveStatistics&) = delete;
  void operator=(const vtkExtractDescriptiveStatistics&) = delete;

  vtkMultiProcessController* Controller{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
