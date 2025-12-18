// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPSciVizPCAStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkGenerateStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPPCAStatistics.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizPCAStats);

vtkPSciVizPCAStats::vtkPSciVizPCAStats()
{
  this->NormalizationScheme = vtkPCAStatistics::NONE;
  this->BasisScheme = vtkPCAStatistics::FULL_BASIS;
  this->FixedBasisSize = 10;
  this->FixedBasisEnergy = 1.;
  this->RobustPCA = false;
}

vtkPSciVizPCAStats::~vtkPSciVizPCAStats() = default;

void vtkPSciVizPCAStats::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NormalizationScheme: " << this->NormalizationScheme << "\n";
  os << indent << "BasisScheme: " << this->BasisScheme << "\n";
  os << indent << "FixedBasisSize: " << this->FixedBasisSize << "\n";
  os << indent << "FixedBasisEnergy: " << this->FixedBasisEnergy << "\n";
  os << indent << "RobustPCA:" << this->RobustPCA << "\n";
}

bool vtkPSciVizPCAStats::PrepareAlgorithm(vtkGenerateStatistics* filter)
{
  if (!filter)
  {
    return false;
  }
  auto pstat = vtkSmartPointer<vtkPCAStatistics>::New();
  pstat->SetNormalizationScheme(this->NormalizationScheme);
  pstat->SetBasisScheme(this->BasisScheme);
  pstat->SetFixedBasisSize(this->FixedBasisSize);
  pstat->SetFixedBasisEnergy(this->FixedBasisEnergy);
  pstat->SetMedianAbsoluteDeviation(this->RobustPCA);
  filter->SetStatisticsAlgorithm(pstat);
  return true;
}
