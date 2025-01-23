// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDistributedTrivialProducer.h"
#include "vtkNew.h"
#include "vtkSphereSource.h"

#include <cstdlib>

extern int TestDistributedTrivialProducer(int, char*[])
{
  vtkNew<vtkSphereSource> sourceData;
  sourceData->Update();
  auto data = sourceData->GetOutput();
  vtkDistributedTrivialProducer::SetGlobalOutput("sphere", data);

  vtkNew<vtkDistributedTrivialProducer> producer;
  producer->UpdateFromGlobal("sphere");
  return producer->GetOutputDataObject(0) == data ? EXIT_SUCCESS : EXIT_FAILURE;
}
