// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPSciVizMultiCorrelativeStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkGenerateStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPMultiCorrelativeStatistics.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizMultiCorrelativeStats);

vtkPSciVizMultiCorrelativeStats::vtkPSciVizMultiCorrelativeStats() = default;

vtkPSciVizMultiCorrelativeStats::~vtkPSciVizMultiCorrelativeStats() = default;

void vtkPSciVizMultiCorrelativeStats::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkPSciVizMultiCorrelativeStats::PrepareAlgorithm(vtkGenerateStatistics* filter)
{
  if (!filter)
  {
    return false;
  }
  auto mstat = vtkSmartPointer<vtkMultiCorrelativeStatistics>::New();
  filter->SetStatisticsAlgorithm(mstat);
  return true;
}
