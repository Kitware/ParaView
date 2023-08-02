// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Los Alamos National Laboratory
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBalancedRedistributePolyData.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBalancedRedistributePolyData);

//----------------------------------------------------------------------------
vtkBalancedRedistributePolyData::vtkBalancedRedistributePolyData() = default;

//----------------------------------------------------------------------------
vtkBalancedRedistributePolyData::~vtkBalancedRedistributePolyData() = default;

//----------------------------------------------------------------------------
void vtkBalancedRedistributePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkWeightedRedistributePolyData::PrintSelf(os, indent);
}

//*****************************************************************
void vtkBalancedRedistributePolyData::MakeSchedule(vtkPolyData* input, vtkCommSched* localSched)

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
  this->Controller->GetLocalProcessId();

  this->SetWeights(0, numProcs - 1, 1.);
  this->vtkWeightedRedistributePolyData::MakeSchedule(input, localSched);
}
//*****************************************************************
