// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "CAdaptorAPI.h"

#include "vtkCPAdaptorAPI.h"

// call at the start of the simulation
void coprocessorinitialize()
{
  vtkCPAdaptorAPI::CoProcessorInitialize();
}

// call at the end of the simulation
void coprocessorfinalize()
{
  vtkCPAdaptorAPI::CoProcessorFinalize();
}

// this is the function that determines whether or not there
// is anything to coprocess this time step
void requestdatadescription(int* timeStep, double* time, int* coprocessThisTimeStep)
{
  vtkCPAdaptorAPI::RequestDataDescription(timeStep, time, coprocessThisTimeStep);
}

// this function sets needgrid to 1 if it does not have a copy of the grid
// it sets needgrid to 0 if it does have a copy of the grid but does not
// check if the grid is modified or needs to be updated
void needtocreategrid(int* needGrid)
{
  vtkCPAdaptorAPI::NeedToCreateGrid(needGrid);
}

// do the actual coprocessing.  it is assumed that the vtkCPDataDescription
// has been filled in elsewhere.
void coprocess()
{
  vtkCPAdaptorAPI::CoProcess();
}
