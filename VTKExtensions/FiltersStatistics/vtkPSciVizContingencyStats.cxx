// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPSciVizContingencyStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkGenerateStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPContingencyStatistics.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizContingencyStats);

vtkPSciVizContingencyStats::vtkPSciVizContingencyStats() = default;

vtkPSciVizContingencyStats::~vtkPSciVizContingencyStats() = default;

void vtkPSciVizContingencyStats::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkPSciVizContingencyStats::PrepareAlgorithm(vtkGenerateStatistics* filter)
{
  if (!filter)
  {
    return false;
  }
  auto cstat = vtkSmartPointer<vtkContingencyStatistics>::New();
  filter->SetStatisticsAlgorithm(cstat);
  return true;
}
