/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeightedPoly.cxx
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
#include "vtkWeightedPoly.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"

vtkCxxSetObjectMacro(vtkWeightedPoly,Controller, vtkMultiProcessController);


vtkWeightedPoly* vtkWeightedPoly::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWeightedPoly");
  if(ret)
    {
    return (vtkWeightedPoly*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWeightedPoly;
}

vtkWeightedPoly::vtkWeightedPoly()
{
  this->Controller = NULL;
  this->Weights= NULL;
}

vtkWeightedPoly::~vtkWeightedPoly()
{
  delete [] Weights;
}

void vtkWeightedPoly::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRedistributePoly::PrintSelf(os,indent);

  os << indent << "Controller: (" << this->Controller << ")\n";
}


//*****************************************************************
void vtkWeightedPoly::SetWeights( const int startProc, const int stopProc, 
                                  const float weight )
{
  int myId, numProcs;
  if (!this->Controller)
    {
    vtkErrorMacro("need controller to set weights");
    return;
    }
  numProcs = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();

  // ... Only set weights on processor 0 to avoid extra communication. ...
  if (myId == 0) 
  {
    int np;
    if (Weights == NULL) 
    {
      Weights = new float[numProcs];
      for (np = 0;  np < numProcs; np++) Weights[np] = 1.;
    }
    for (np = startProc;  np <= stopProc; np++) Weights[np] = weight;
  }
}

//*****************************************************************
void vtkWeightedPoly::MakeSchedule ( vtkCommSched& localSched)

{
//*****************************************************************
// purpose: This routine sets up a schedule to shift cells around so
//          the number of cells on each processor is as even as possible.
//
//*****************************************************************

  // get total number of polys and figure out how many each processor should have

  vtkPolyData *input = this->GetInput();
  vtkCellArray *polys = input->GetPolys();
  vtkIdType numLocalCells = polys->GetNumberOfCells(); 

  //int avgCells = 0;
  vtkIdType leftovers = 0;

  int i;
  int myId, numProcs;
  if (!this->Controller)
    {
    vtkErrorMacro("need controller to balance cells");
    return;
    }
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  int id;
  if (myId == 0)
  {
    if (Weights == NULL) 
    {
      // ... no weights have been set so this is a uniform balance case ...
      Weights = new float[numProcs];
      for (id = 0;  id < numProcs; id++) Weights[id] = 1./ numProcs;
    }
    else
    {
      // ... weights have been set so normalize to add to 1. ...
      float weight_sum = 0.;
      for (id=0; id<numProcs; id++) weight_sum += Weights[id];

      float weight_scale = 0.;
      if (weight_sum>0) weight_scale = 1. / weight_sum;
      for (id=0; id<numProcs; id++) Weights[id] *= weight_scale;
    }
  }

  vtkCommSched* remoteSched = new vtkCommSched[numProcs];

  vtkIdType totalCells;
  vtkIdType numRemoteCells;
  vtkIdType* goalNumCells;
  int numProcZero = 0;
  if (myId!=0)
  {
     this->Controller->
        Send((vtkIdType*)(&numLocalCells), 1, 0, VTK_NUM_LOC_CELLS_TAG); 
  } 
  else
  {
    totalCells = numLocalCells;
    remoteSched[0].numCells = numLocalCells;
    for (id = 1; id < numProcs; id++)
    {
      this->Controller->Receive((vtkIdType*)(&numRemoteCells), 1, id,
				 VTK_NUM_LOC_CELLS_TAG);
      totalCells += numRemoteCells;
      remoteSched[id].numCells = numRemoteCells;
    }
    goalNumCells = new vtkIdType [numProcs]; 
    for (id = 0; id < numProcs; id++) 
    {
      goalNumCells[id] = totalCells * Weights[id];
      if (goalNumCells[id]==0) numProcZero++;
    }

    vtkIdType sumCells = 0;
    for (id = 0; id < numProcs; id++) sumCells += goalNumCells[id];
    leftovers = totalCells - sumCells;
  }

  // ... order processors to minimize the number of sends (replace with more 
  //   efficient sort later) ... 
  int schedLen1;
  int schedLen2;
  int* schedArray1;
  vtkIdType* schedArray2;

  int temp;
  if (myId==0)
  {
    int *order = new int[numProcs];
    int id2;
    for (id = 0; id < numProcs; id++) order[id] = id;
    for (id = 0; id < numProcs; id++)
    {
       for (id2 = id+1; id2 < numProcs; id2++)
       {
          //if (remoteSched[order[id]].numCells < remoteSched[order[id2]].numCells)
          if ((remoteSched[order[id]].numCells - goalNumCells[order[id]]) < 
              (remoteSched[order[id2]].numCells - goalNumCells[order[id2]]))
          {
             temp = order[id];
             order[id] = order[id2];
             order[id2] = temp;
          }
       }
    }
    // now figure out what processors to send cells between
    int *sendToTemp  = new int[numProcs];
    vtkIdType *sendNumTemp = new vtkIdType[numProcs];

    int start = 0;
    int last = numProcs-1;
    int cnt = 0;

    int recflag = 0; // turn on if receive should get extra leftover cell,
                     // without flag this gets lost
    for (id = 0; id < numProcs; id++) remoteSched[id].cntSend = 0;

    vtkIdType numToSend, numToReceive;
    //while (start<last && cnt<1)
    while (start<last)
    {
       //numToSend = remoteSched[order[start]].numCells-avgCells;
       numToSend = remoteSched[order[start]].numCells-goalNumCells[order[start]];

       // put in special test for when weights are exactly 0 to make
       // sure all points are removed
       if ((leftovers)>0 && (goalNumCells[order[start]]!=0))
       {
         numToSend--;
         leftovers--;
       }
       cnt = 0;
       //while (numToSend>0 && cnt<2)
       //if (numToSend>0) numToSend=1;
       //while (numToSend>0 && cnt<1)
       while (numToSend>0)
       {
          if (start == last) cerr<<"error: start =last"<<endl;
          //numToReceive = avgCells-remoteSched[order[last]].numCells;
          numToReceive = goalNumCells[order[last]]-remoteSched[order[last]].numCells;
          if (leftovers>=(last-start-numProcZero) || (recflag==1))
          {
            // ... receiving processors are going to get some of the 
            //   leftover cells too ...
            numToReceive++;
            if (!recflag) leftovers--;  // if flag, this has already been
                                        // subtracted
            recflag = 1;  
          }
          if (numToSend >= numToReceive)
          {
            sendToTemp[cnt] = order[last];
            sendNumTemp[cnt++] = numToReceive;
            numToSend -= numToReceive;
            remoteSched[order[start]].numCells -= numToReceive;
            remoteSched[order[last]].numCells  += numToReceive;
            last--;
            recflag = 0; // extra leftover point will have been used
          }
          else
          {
            sendToTemp[cnt] = order[last];
            sendNumTemp[cnt++] = numToSend;
            remoteSched[order[start]].numCells -= numToSend;
            remoteSched[order[last]].numCells  += numToSend;
            numToSend = 0;
          }
       }
       int ostart = order[start];
       remoteSched[ostart].cntSend = cnt;
       remoteSched[ostart].sendTo  = new int[cnt];
       remoteSched[ostart].sendNum = new vtkIdType[cnt];
       for (int i=0; i<cnt;i++)
       {
          remoteSched[ostart].sendTo[i]  = sendToTemp[i];
          remoteSched[ostart].sendNum[i] = sendNumTemp[i];
       }
       start++;
    }

    for (id = 0; id < numProcs; id++) remoteSched[id].cntRec = 0; 

    // ... count up how processors a processor will be receiving from ...
    for (id = 0; id < numProcs; id++) 
      for (i=0;i<remoteSched[id].cntSend;i++) 
      {
         int sendId = remoteSched[id].sendTo[i];
         remoteSched[sendId].cntRec++; 
      }

    // ... allocate memory to store the processor to receive from and how 
    //   many to receive ...
    for (id = 0; id < numProcs; id++) 
    {
      remoteSched[id].recFrom = new int [remoteSched[id].cntRec]; 
      remoteSched[id].recNum  = new vtkIdType [remoteSched[id].cntRec]; 
    }

    // ... reinitialize the count so records are stored in the correct place ...
    for (id = 0; id < numProcs; id++) remoteSched[id].cntRec = 0; 

    // ... store which processors will be received from and how many cells 
    //   will be received ...
    for (id = 0; id < numProcs; id++) 
    {
       for (i=0;i<remoteSched[id].cntSend;i++) 
       {
         int recId = remoteSched[id].sendTo[i];
         int remCntRec = remoteSched[recId].cntRec;
         remoteSched[recId].recFrom[remCntRec] = id; 
         remoteSched[recId].recNum[remCntRec] = remoteSched[id].sendNum[i]; 
         remoteSched[recId].cntRec++;
       }
    }

    // ... now send the schedule as an array of ints (all rec are ints so 
    //     this is okay) ...
    // ... not any more! ...
    for (id = 1; id < numProcs; id++)
    {
      // ... number of ints ...
      schedLen1 = 2 + remoteSched[id].cntSend + remoteSched[id].cntRec;

      // ... number of vtkIdTypes ...
      schedLen2 = 1 + remoteSched[id].cntSend + remoteSched[id].cntRec;

      schedArray1 = new int [schedLen1];
      schedArray2 = new vtkIdType [schedLen2];
      schedArray2[0] = remoteSched[id].numCells;
      schedArray1[0] = remoteSched[id].cntSend;
      schedArray1[1] = remoteSched[id].cntRec;
      int arraycnt1 = 2;
      int arraycnt2 = 1;
      if (remoteSched[id].cntSend > 0)
      {
        for (i=0;i<remoteSched[id].cntSend;i++) 
        {
           schedArray1[arraycnt1++] = remoteSched[id].sendTo[i];
           schedArray2[arraycnt2++] = remoteSched[id].sendNum[i];
        }
      }
      if (remoteSched[id].cntRec > 0)
      {
        for (i=0;i<remoteSched[id].cntRec;i++) 
        {
           schedArray1[arraycnt1++] = remoteSched[id].recFrom[i];
           schedArray2[arraycnt2++] = remoteSched[id].recNum[i];
        }
      }
      this->Controller->Send((int*)(&schedLen1), 1, id, VTK_SCHED_LEN_1_TAG); 
      this->Controller->Send((int*)(&schedLen2), 1, id, VTK_SCHED_LEN_2_TAG); 
      this->Controller->Send((int*)schedArray1, schedLen1, id, VTK_SCHED_1_TAG); 
      this->Controller->Send((vtkIdType*)schedArray2, schedLen2, id, VTK_SCHED_2_TAG); 
      delete [] schedArray1;
      delete [] schedArray2;
    } 

    // ... initialize the local schedule to return ...
    localSched.numCells = remoteSched[0].numCells;
    localSched.cntSend  = remoteSched[0].cntSend;
    localSched.cntRec   = remoteSched[0].cntRec;
    localSched.sendTo   = new int [localSched.cntSend];
    localSched.recFrom  = new int [localSched.cntRec];
    localSched.sendNum  = new vtkIdType [localSched.cntSend];
    localSched.recNum   = new vtkIdType [localSched.cntRec];
    for (i=0;i<localSched.cntSend; i++)
    {
      localSched.sendTo[i]  = remoteSched[0].sendTo[i];
      localSched.sendNum[i] = remoteSched[0].sendNum[i];
    }
    for (i=0;i<localSched.cntRec; i++)
    {
      localSched.recFrom[i] = remoteSched[0].recFrom[i];
      localSched.recNum[i]  = remoteSched[0].recNum[i];
    }

    delete [] order;
    delete [] sendToTemp;
    delete [] sendNumTemp;
    delete [] goalNumCells;
  }
  else
  {
    // myId != 0
    schedLen1;
    schedLen2;
    this->Controller->Receive((int*)(&schedLen1), 1, 0, VTK_SCHED_LEN_1_TAG); 
    this->Controller->Receive((int*)(&schedLen2), 1, 0, VTK_SCHED_LEN_2_TAG); 
    schedArray1       = new int [schedLen1];
    schedArray2 = new vtkIdType [schedLen2];
    this->Controller->Receive(schedArray1, schedLen1, 0, VTK_SCHED_1_TAG); 
    this->Controller->Receive(schedArray2, schedLen2, 0, VTK_SCHED_2_TAG); 

    localSched.numCells = schedArray2[0];
    localSched.cntSend  = schedArray1[0];
    localSched.cntRec   = schedArray1[1];
    int arraycnt1 = 2;
    int arraycnt2 = 1;
    if (localSched.cntSend > 0)
    {
      localSched.sendTo  = new int [localSched.cntSend];
      localSched.sendNum = new vtkIdType [localSched.cntSend];
      for (i=0;i<localSched.cntSend;i++) 
      {
         localSched.sendTo[i]  = schedArray1[arraycnt1++];
         localSched.sendNum[i] = schedArray2[arraycnt2++];
      }
    }
    if (localSched.cntRec > 0)
    {
      localSched.recFrom = new int [localSched.cntRec];
      localSched.recNum  = new vtkIdType [localSched.cntRec];
      for (i=0;i<localSched.cntRec;i++) 
      {
         localSched.recFrom[i] = schedArray1[arraycnt1++]; 
         localSched.recNum[i]  = schedArray2[arraycnt2++];
      }
    }
    delete [] schedArray1;
    delete [] schedArray2;
  }
}
//*****************************************************************
