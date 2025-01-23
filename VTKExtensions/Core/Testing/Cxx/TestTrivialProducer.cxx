// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVTrivialProducer.h"

#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"

namespace
{
void verifyOutput(vtkInformation* info, int size, double last)
{
  int length = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  vtkLogIf(
    ERROR, length != size, "Wrong number of timesteps! Has " << length << " instead of " << size);
  auto times = info->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  vtkLogIf(ERROR, times[1] != last,
    "Incorrect value for last timestep. Has " << times[1] << " instead of " << last);
}
};

extern int TestTrivialProducer(int, char*[])
{
  vtkNew<vtkSphereSource> sourceData;
  sourceData->Update();
  auto data = sourceData->GetOutput();

  vtkNew<vtkPVTrivialProducer> producer;
  producer->SetOutput(data, -2);
  producer->Update();

  auto info = producer->GetOutputInformation(0);
  assert(info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
  assert(info->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()));

  ::verifyOutput(info, 1, -2);

  producer->SetOutput(data, 0.5);
  producer->Update();
  ::verifyOutput(info, 2, 0.5);

  // insert between first and last should raise a warning but is OK.
  producer->SetOutput(data, 0.2);
  producer->Update();
  ::verifyOutput(info, 3, 0.5);

  // reinsert existing data should not add value in TIME_STEPS.
  producer->SetOutput(data, 0.2);
  producer->Update();
  ::verifyOutput(info, 3, 0.5);

  // setting data without timesteps
  producer->SetOutput(data);
  producer->Update();
  ::verifyOutput(info, 3, 0.5);

  return EXIT_SUCCESS;
}
