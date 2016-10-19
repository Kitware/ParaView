/*=========================================================================

  Program:   ParaView
  Module:    vtkBalancedRedistributePolyData.cxx

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

#include "vtkBalancedRedistributePolyData.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBalancedRedistributePolyData);

//----------------------------------------------------------------------------
vtkBalancedRedistributePolyData::vtkBalancedRedistributePolyData()
{
}

//----------------------------------------------------------------------------
vtkBalancedRedistributePolyData::~vtkBalancedRedistributePolyData()
{
}

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
