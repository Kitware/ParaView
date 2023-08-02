// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPSciVizDescriptiveStats
 * @brief   Provide access to VTK descriptive statistics.
 *
 * This filter provides access to the features of vtkDescriptiveStatistics.
 * See VTK documentation for details
 *
 * @par Thanks:
 * Thanks to David Thompson and Philippe Pebay from Sandia National Laboratories
 * for implementing this class.
 */

#ifndef vtkPSciVizDescriptiveStats_h
#define vtkPSciVizDescriptiveStats_h

#include "vtkPVVTKExtensionsFiltersStatisticsModule.h" //needed for exports
#include "vtkSciVizStatistics.h"

class VTKPVVTKEXTENSIONSFILTERSSTATISTICS_EXPORT vtkPSciVizDescriptiveStats
  : public vtkSciVizStatistics
{
public:
  static vtkPSciVizDescriptiveStats* New();
  vtkTypeMacro(vtkPSciVizDescriptiveStats, vtkSciVizStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(SignedDeviations, int);
  vtkGetMacro(SignedDeviations, int);

protected:
  vtkPSciVizDescriptiveStats();
  ~vtkPSciVizDescriptiveStats() override;

  int LearnAndDerive(vtkMultiBlockDataSet* model, vtkTable* inData) override;
  int AssessData(
    vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model) override;

  int SignedDeviations;

private:
  vtkPSciVizDescriptiveStats(const vtkPSciVizDescriptiveStats&) = delete;
  void operator=(const vtkPSciVizDescriptiveStats&) = delete;
};

#endif // vtkPSciVizDescriptiveStats_h
