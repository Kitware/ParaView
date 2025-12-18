// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPSciVizDescriptiveStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkGenerateStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPDescriptiveStatistics.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizDescriptiveStats);

vtkPSciVizDescriptiveStats::vtkPSciVizDescriptiveStats()
{
  this->SignedDeviations = 0;
}

vtkPSciVizDescriptiveStats::~vtkPSciVizDescriptiveStats() = default;

void vtkPSciVizDescriptiveStats::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SignedDeviations: " << this->SignedDeviations << "\n";
}

bool vtkPSciVizDescriptiveStats::PrepareAlgorithm(vtkGenerateStatistics* filter)
{
  if (!filter)
  {
    return false;
  }
  auto dstat = vtkSmartPointer<vtkDescriptiveStatistics>::New();
  dstat->SetSignedDeviations(this->SignedDeviations);
  filter->SetStatisticsAlgorithm(dstat);
  return true;
}
