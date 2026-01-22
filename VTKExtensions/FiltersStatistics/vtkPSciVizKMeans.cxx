// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPSciVizKMeans.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkGenerateStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPKMeansStatistics.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizKMeans);

vtkPSciVizKMeans::vtkPSciVizKMeans()
{
  this->K = 5;
  this->MaxNumIterations = 50;
  this->Tolerance = 0.01;
}

vtkPSciVizKMeans::~vtkPSciVizKMeans() = default;

void vtkPSciVizKMeans::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "K: " << K << "\n";
  os << indent << "MaxNumIterations: " << this->MaxNumIterations << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

bool vtkPSciVizKMeans::PrepareAlgorithm(vtkGenerateStatistics* filter)
{
  if (!filter)
  {
    return false;
  }
  auto kstat = vtkSmartPointer<vtkKMeansStatistics>::New();
  kstat->SetDefaultNumberOfClusters(this->K);
  kstat->SetMaxNumIterations(this->MaxNumIterations);
  kstat->SetTolerance(this->Tolerance);
  filter->SetStatisticsAlgorithm(kstat);
  return true;
}
