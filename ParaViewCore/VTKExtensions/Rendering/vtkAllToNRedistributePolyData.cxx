/*=========================================================================

  Program:   ParaView
  Module:    vtkAllToNRedistributePolyData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Los Alamos National Laboratory
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

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
vtkAllToNRedistributePolyData::~vtkAllToNRedistributePolyData()
{
}

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
