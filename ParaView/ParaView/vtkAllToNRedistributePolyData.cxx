/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAllToNRedistributePolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/*======================================================================
// This software and ancillary information known as vtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.
======================================================================*/


#define DO_TIMING 0
#include "vtkAllToNRedistributePolyData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"

vtkAllToNRedistributePolyData* vtkAllToNRedistributePolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAllToNRedistributePolyData");
  if(ret)
    {
    return (vtkAllToNRedistributePolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAllToNRedistributePolyData;
}

vtkAllToNRedistributePolyData::vtkAllToNRedistributePolyData()
{
  this->NumberOfProcesses = 1;
}

vtkAllToNRedistributePolyData::~vtkAllToNRedistributePolyData()
{
}

void vtkAllToNRedistributePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkWeightedRedistributePolyData::PrintSelf(os,indent);
  os << indent << "Number of processes: " << this->NumberOfProcesses << endl;
}


//*****************************************************************
void vtkAllToNRedistributePolyData::MakeSchedule ( vtkCommSched* localSched)

{
//*****************************************************************
// purpose: This routine sets up a schedule to shift cells around so
//          the number of cells on each processor is as even as possible.
//
//*****************************************************************

  // get total number of polys and figure out how many each processor should have

  int myId, numProcs;
  if (!this->Controller)
    {
    vtkErrorMacro("need controller to set weights");
    return;
    }

  numProcs = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();


  // make sure the cells are redistributed into a valid range.
  int numberOfValidProcesses = this->NumberOfProcesses;
  if (numberOfValidProcesses <= 0) { numberOfValidProcesses = numProcs; }
  if (numberOfValidProcesses > numProcs ) { numberOfValidProcesses = numProcs; }

  this->SetWeights(0, numberOfValidProcesses-1, 1.);
  if (numberOfValidProcesses < numProcs)
    {
    this->SetWeights(numberOfValidProcesses, numProcs-1, 0.);
    }

  this->vtkWeightedRedistributePolyData::MakeSchedule(localSched);

}
//*****************************************************************
