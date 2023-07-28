// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Los Alamos National Laboratory
// SPDX-License-Identifier: BSD-3-Clause
#define DO_TIMING 0
#include "vtkAllToNRedistributePolyData.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAllToNRedistributePolyData);

//----------------------------------------------------------------------------
vtkAllToNRedistributePolyData::vtkAllToNRedistributePolyData()
{
  this->NumberOfProcesses = 1;
}

//----------------------------------------------------------------------------
vtkAllToNRedistributePolyData::~vtkAllToNRedistributePolyData() = default;

//----------------------------------------------------------------------------
void vtkAllToNRedistributePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of processes: " << this->NumberOfProcesses << endl;
}

//*****************************************************************
void vtkAllToNRedistributePolyData::MakeSchedule(vtkPolyData* input, vtkCommSched* localSched)

{
  //*****************************************************************
  // purpose: This routine sets up a schedule to shift cells around so
  //          the number of cells on each processor is as even as possible.
  //
  //*****************************************************************

  // get total number of polys and figure out how many each processor should have

  int numProcs;
  if (!this->Controller)
  {
    vtkErrorMacro("need controller to set weights");
    return;
  }

  numProcs = this->Controller->GetNumberOfProcesses();

  // make sure the cells are redistributed into a valid range.
  int numberOfValidProcesses = this->NumberOfProcesses;
  if (numberOfValidProcesses <= 0)
  {
    numberOfValidProcesses = numProcs;
  }
  if (numberOfValidProcesses > numProcs)
  {
    numberOfValidProcesses = numProcs;
  }

  this->SetWeights(0, numberOfValidProcesses - 1, 1.);
  if (numberOfValidProcesses < numProcs)
  {
    this->SetWeights(numberOfValidProcesses, numProcs - 1, 0.);
  }

  this->Superclass::MakeSchedule(input, localSched);
}
//*****************************************************************
