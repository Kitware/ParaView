/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRedistributePoly.cxx
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

/*================================================================ ====
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
=========================================================================*/

#define DO_TIMING 0
#include "vtkRedistributePoly.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkPolyDataWriter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkCharArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkLongArray.h"

#include "vtkShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkDataSetAttributes.h"

#include "vtkTimerLog.h"
//#include "mpi.h"

#include "vtkMultiProcessController.h"

vtkCxxSetObjectMacro(vtkRedistributePoly,Controller, vtkMultiProcessController);


typedef struct {vtkTimerLog* timer; float time;} _TimerInfo;
_TimerInfo timerInfo8;

vtkRedistributePoly* vtkRedistributePoly::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRedistributePoly");
  if(ret)
    {
    return (vtkRedistributePoly*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRedistributePoly;
}

vtkRedistributePoly::vtkRedistributePoly()
{
  this->Controller = NULL;
  //this->Locator = vtkPointLocator::New();
  this->colorProc = 0;
}

vtkRedistributePoly::~vtkRedistributePoly()
{
  // //this->SetController(NULL);
  //this->Locator->Delete();
  //this->Locator = NULL;
}

void vtkRedistributePoly::Execute()
{
#if (DO_TIMING==1) 
  vtkTimerLog* timer8 = vtkTimerLog::New();
  timerInfo8.timer = timer8;

  //timerInfo8.time = 0.;
  timerInfo8.timer->StartTimer();
#endif

  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  int myId;
  if (!this->Controller)
     this->Controller = vtkMultiProcessController::GetGlobalController();

  if (!this->Controller)
  {
    vtkErrorMacro("need controller to redistribute cells");
    return;
  }
  this->Controller->Register(this);
  myId = this->Controller->GetLocalProcessId();


  // ... make schedule of how many and where to ship polys ...

  this->Controller->Barrier();
  //MPI_Barrier(MPI_COMM_WORLD);
#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"barrier bef sched time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif


  vtkCommSched* localSched = vtkCommSched::New(); 
  MakeSchedule ( *localSched ); 
  OrderSchedule ( *localSched);  // order schedule to avoid 
                                         // blocking problems later
  vtkIdType **sendCellList = localSched->sendCellList; 
  vtkIdType *keepCellList  = localSched->keepCellList; 
  int *sendTo  = localSched->sendTo;
  int *recFrom = localSched->recFrom; 
  int cntSend  = localSched->cntSend;
  int cntRec   = localSched->cntRec;
  vtkIdType *sendNum = localSched->sendNum; 
  vtkIdType *recNum  = localSched->recNum; 
  vtkIdType numCells = localSched->numCells;
  
#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"schedule time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  

// beginning of turned of bounds section (not needed)
#if 0
  // ... expand bounds on all processors to be the maximum on any processor ...

  this->Controller->Barrier();
  //MPI_Barrier(MPI_COMM_WORLD);
#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"barrier bef bounds time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif


  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();

  float *bounds = input->GetBounds(), *remoteBounds = bounds;
  //cerr<<"myId="<<myId<<",bounds[0:5]=" <<bounds[0]<<", "<<bounds[1]<<", "
  //    <<bounds[2]<<", "<<bounds[3]<<", " <<bounds[4]<<", "<<bounds[5]<<endl;

  for (id = 0; id < numProcs; id++)
    {
    // ... send out bounds to all the other processors ...
    if (id != myId)
      {
      this->Controller->Send(input->GetBounds(), 6, id, VTK_BOUNDS_TAG);
      }
    }
  
  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
      // ... get remote bounds and expand bounds to include these ...
      this->Controller->Receive(remoteBounds, 6, id, VTK_BOUNDS_TAG);
      if (remoteBounds[0] < bounds[0]) bounds[0] = remoteBounds[0];
      if (remoteBounds[1] > bounds[1]) bounds[1] = remoteBounds[1];
      if (remoteBounds[2] < bounds[2]) bounds[2] = remoteBounds[2];
      if (remoteBounds[3] > bounds[3]) bounds[3] = remoteBounds[3];
      if (remoteBounds[4] < bounds[4]) bounds[4] = remoteBounds[4];
      if (remoteBounds[5] > bounds[5]) bounds[5] = remoteBounds[5];
      }
    }
  
#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<" bounds time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif


#endif   // end of turned of bounds section


  // ... allocate space and copy point and cell data attributes from input to 
  //   output ...

  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetCellData()->CopyAllocate(input->GetCellData());
  
  // ... make sure output array info is initialized  ...

  int getArrayInfo = 0;
  int sendArrayInfo;

  //if (input->GetPointData()->GetNumberOfArrays() == 0 || 
  //    input->GetCellData()->GetNumberOfArrays() == 0)
  if (input->GetPointData()->GetNumberOfArrays() == 0 ) 
  {
    // set request flag to true ...
    getArrayInfo = 1;
  }
  else
  {
    // set request flag to false because the array info isn't needed ...
    getArrayInfo = 0;
  }
  if (cntRec > 0) 
    this->Controller->Send(&getArrayInfo, 1, recFrom[0], 997243);

  // ... send false request flags to the rest of the processors data is being 
  //   received from ...

  int i;
  int getArrayInfo2 = 0;
  for (i=1; i<cntRec; i++)
  {
    this->Controller->Send(&getArrayInfo2, 1, recFrom[i], 997243);
  }

  // ... loop over all processors data is being sent to and send array 
  //   information if it is needed ...

  for (i=0; i<cntSend; i++)
  {
    // ... get flag ...
    this->Controller->Receive(&sendArrayInfo, 1, sendTo[i], 997243);

     if (sendArrayInfo)
       SendCompleteArrays(sendTo[i]);
  }


  // ... receive array info from first array in recFrom list ...

  if (cntRec>0 && getArrayInfo)
    CompleteArrays(recFrom[0]);


  // ... copy remaining input cell data to output cell data ...

  vtkCellArray *polys = input->GetPolys();

  vtkIdType inputNumCells = polys->GetNumberOfCells();
  vtkIdType origNumCells = inputNumCells;
  // check to see if number of cells is less than original and only copy that 
  // many or copy all of input cells if extra are added
  //cerr<<"myId="<<myId<<", orig num cells ="<<origNumCells<<", num after balance= "
  //    <<numCells<<endl;
  if (numCells<origNumCells) origNumCells = numCells; 

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"copy alloctime = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  //ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
    // ... send cell and point sizes ...

  vtkIdType  prevStopCell = origNumCells - 1;
  vtkIdType  startCell,stopCell;

  vtkIdType *numPointsSend = new vtkIdType[cntSend];
  vtkIdType *cellArraySize = new vtkIdType[cntSend];

  for (i=0; i<cntSend; i++)
  {
     if (sendCellList == NULL)
     {
       startCell = prevStopCell+1;
       stopCell = startCell+sendNum[i]-1;
       SendCellSizes (startCell, stopCell, input, sendTo[i], 
                      numPointsSend[i], cellArraySize[i], NULL);
     }
     else
     {
        startCell = 0;
        stopCell = sendNum[i]-1;
        SendCellSizes (startCell, stopCell, input, sendTo[i], 
                       numPointsSend[i], cellArraySize[i], sendCellList[i]);
     }

     prevStopCell = stopCell;

  } // end of list of processors to send to

  //ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
  //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
// ... allocate memory before receiving data ...

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"send sizes time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  // ... find memory requirements for on processor copy ...
  vtkIdType numPointsOnProc = 0;
  vtkIdType numCellPtsOnProc = 0;
  FindMemReq(origNumCells, input, numPointsOnProc, numCellPtsOnProc);

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"mem req time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  vtkIdType* numPointsRec = new vtkIdType[cntRec];
  vtkIdType* cellptCntr = new vtkIdType[cntRec];

  for (i=0; i<cntRec; i++)
  {
     this->Controller->Receive((vtkIdType*)&cellptCntr[i], 1, recFrom[i],
                                VTK_CELL_CNT_TAG);
     this->Controller->Receive ((vtkIdType*)&numPointsRec[i], 1, recFrom[i],
                                VTK_POINTS_SIZE_TAG);
  }

  vtkCellData* outputCellData   = output->GetCellData();
  vtkPointData* outputPointData = output->GetPointData();

  AllocateDataArrays (outputCellData, recNum, cntRec, 
                      recFrom, origNumCells );
  AllocateDataArrays (outputPointData, numPointsRec, cntRec, 
                      recFrom, numPointsOnProc);

  vtkIdType totalNumPoints = numPointsOnProc;
  vtkIdType totalNumCells = origNumCells;
  vtkIdType totalNumCellPts = numCellPtsOnProc;
  for (i=0; i<cntRec; i++)
  {
     totalNumPoints += numPointsRec[i];
     totalNumCells += recNum[i];
     totalNumCellPts += cellptCntr[i];
  }

  //vtkPoints *inputPoints = input->GetPoints();
  vtkPoints *outputPoints = vtkPoints::New();
  outputPoints->SetNumberOfPoints(totalNumPoints);

  vtkCellArray *outputPolys = vtkCellArray::New();;
  vtkIdType* ptr = 0; 
  if (totalNumCellPts >0)
  {
     ptr = outputPolys->WritePointer(totalNumCells,totalNumCellPts);
     if (ptr == 0) cerr<<"Error: can't allocate points."<<endl;
  }

  output->SetPolys(outputPolys);
  output->SetPoints(outputPoints);

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"alloc time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
  // ... Copy cells from input to output ...
  CopyCells(origNumCells, input, output, keepCellList);

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"copy cells time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif
  //eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee

  // ... first exchange cells between processors.  Do this by receiving first if
  //   this processor number is less than the one it is exchanging with else 
  //   send first ...
  
  vtkIdType prevStopCellRec = origNumCells - 1;
  vtkIdType prevStopCellSend = origNumCells - 1;

  vtkIdType  prevNumPointsRec = numPointsOnProc;

  vtkIdType  prevCellptCntrRec = numCellPtsOnProc;

  int finished = 0;
  int procRec,procSend;
  int rcntr=0;
  int scntr=0;
  int receiving;

  while (!finished && (cntRec>0 || cntSend>0))
  {
     if (rcntr<cntRec)
        procRec = recFrom[rcntr];
     else
        procRec = 99999;
     if (scntr<cntSend)
        procSend = sendTo[scntr];
     else
        procSend = 99999;

     receiving = 0;

     // ... send or receive the smallest processor number next 
     //   (will send if receive == 0) ...
     if (procRec<procSend)
        receiving = 1;
     else if (procRec==procSend)
     {
        // ... an exchange between 2 prcessors ...
        if (myId < procRec) receiving = 1;
     }

     if (receiving)
     {
        startCell = prevStopCellRec+1;
        stopCell = startCell+recNum[rcntr]-1;

        ReceiveCells (startCell, stopCell, output, recFrom[rcntr],
                      prevCellptCntrRec, cellptCntr[rcntr], prevNumPointsRec,
                      numPointsRec[rcntr]);

        prevNumPointsRec += numPointsRec[rcntr];
        prevCellptCntrRec += cellptCntr[rcntr];
        prevStopCellRec = stopCell;
        rcntr++;
     }
     else
     {
        // ... sending ...
        if (sendCellList == NULL)
        {
           startCell = prevStopCellSend+1;
           stopCell = startCell+sendNum[scntr]-1;
           SendCells (startCell, stopCell, input, output, sendTo[scntr], 
                      numPointsSend[scntr], cellArraySize[scntr], NULL);
        }
        else
        {
           startCell = 0;
           stopCell = sendNum[scntr]-1;
           SendCells (startCell, stopCell, input, output, sendTo[scntr], 
                      numPointsSend[scntr], cellArraySize[scntr], 
                      sendCellList[scntr]);
        }
 
        prevStopCellSend = stopCell;
        scntr++;
      }
     
      if (scntr>=cntSend && rcntr>=cntRec) finished = 1;
  }
  //eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee

  // Put delete here to avoid copying localSched fields.  If not copied
  // and deleted earlier then arrays will be stmped.

  localSched->Delete();

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)cerr<<"send/rec (at end) time = "<<timerInfo8.time<<endl;
#endif
}

void vtkRedistributePoly::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Controller (" << this->Controller << ")\n";
  //os << indent << "Locator (" << this->Locator << ")/n";
}


//*****************************************************************
void vtkRedistributePoly::MakeSchedule ( vtkCommSched& localSched)

{
//*****************************************************************
// purpose: This routine sets up a schedule that does nothing.
//
//*****************************************************************

  // get total number of polys and figure out how many each processor should have

  vtkPolyData *input = this->GetInput();
  vtkCellArray *polys = input->GetPolys();
  vtkIdType numLocalCells = polys->GetNumberOfCells(); 

  // ... order processors to minimize the number of sends (replace with more 
  //   efficient sort later) ... 
  // ... initialize the local schedule to return ...
  localSched.numCells = numLocalCells;
  localSched.cntSend  = 0;
  localSched.cntRec   = 0;
  localSched.sendTo   = NULL;
  localSched.sendNum  = NULL;
  localSched.recFrom  = NULL;
  localSched.recNum   = NULL;
  localSched.sendCellList = NULL;
  localSched.keepCellList = NULL;

}
//*****************************************************************
void vtkRedistributePoly::OrderSchedule ( vtkCommSched& localSched)

{
  vtkIdType **sendCellList = localSched.sendCellList; 
  vtkIdType *sendNum = localSched.sendNum; 
  vtkIdType *recNum  = localSched.recNum; 
  int *sendTo  = localSched.sendTo;
  int *recFrom = localSched.recFrom; 
  int cntSend  = localSched.cntSend;
  int cntRec   = localSched.cntRec;
  
  // ... first find number of exchanges where a processor both sends and receives
  //  from the same processor ...

  int i,j;
  int* order;
  int temp;
  int tempid;
  vtkIdType* templist;
  int temporder;


  // ... first order sends and then receives to avoid blocking problems later ...
 
  int outoforder;  // flag to determine if schedule is out of order
  if (cntSend>0)
  {
     outoforder=0;
     order = new int[cntSend];
     for (i = 0; i<cntSend; i++) order[i] = i; 
     for (i = 0; i<cntSend; i++) 
       for (j = i+1; j<cntSend; j++) 
          if (sendTo[i] > sendTo[j])
          {
             temp = order[i];
             order[i] = order[j];
             order[j] = temp;
             outoforder=1;
          }
     // ... now reorder the sends ...
     if (outoforder)
     {
       for (i = 0; i<cntSend; i++) 
       {
          while (order[i] != i)
          {
             temporder = order[i];

             temp = sendTo[i];
             sendTo[i] = sendTo[temporder];
             sendTo[temporder] = temp;

             tempid = sendNum[i];
             sendNum[i] = sendNum[temporder];
             sendNum[temporder] = tempid;

             if (sendCellList != NULL)
             {
               templist = sendCellList[i];
               sendCellList[i] = sendCellList[temporder];
               sendCellList[temporder] = templist;
             }

             temporder = order[i];
             order[i] = order[temporder];
             order[temporder] = temporder;
        
          }
       }
     }
     delete [] order;
  }
  if (cntRec>0)
  {
     outoforder=0;
     order = new int[cntRec];
     for (i = 0; i<cntRec; i++) order[i] = i; 
     for (i = 0; i<cntRec; i++) 
       for (j = i+1; j<cntRec; j++) 
          if (recFrom[i] > recFrom[j])
          {
             temp = order[i];
             order[i] = order[j];
             order[j] = temp;
             outoforder=1;
          }
     // ... now reorder the receives ...
     if (outoforder)
     {
       for (i = 0; i<cntRec; i++) 
       {
          while (order[i] != i)
          {
             temporder = order[i];

             temp = recFrom[i];
             recFrom[i] = recFrom[temporder];
             recFrom[temporder] = temp;

             tempid = recNum[i];
             recNum[i] = recNum[temporder];
             recNum[temporder] = tempid;

             temporder = order[i];
             order[i] = order[temporder];
             order[temporder] = temporder;
        
          }
       }
     }
     delete [] order;
  }
}
//*****************************************************************
//*****************************************************************
// Copy the attribute data from one id to another. Make sure CopyAllocate() has// been invoked before using this method.
void vtkRedistributePoly::CopyDataArrays
   (vtkDataSetAttributes* fromPd, vtkDataSetAttributes* toPd,
    const vtkIdType numToCopy, vtkIdType* fromId, const int myId)
{

  vtkDataArray* DataFrom;
  vtkDataArray* DataTo;

  if ( toPd->GetCopyScalars() )
  {
    vtkDataArray* fromScalars = fromPd->GetScalars();
    vtkDataArray* toScalars = toPd->GetScalars();
    if (fromScalars != NULL)
    {
      DataFrom = fromScalars;
      DataTo = toScalars;
      int activeComponent = 0;
      //int activeComponent = fromScalars->GetActiveComponent();
      CopyArrays (DataFrom, DataTo, numToCopy, fromId, activeComponent, myId);
    }
  }

  if ( toPd->GetCopyVectors() )
  {
    vtkDataArray* fromVectors = fromPd->GetVectors();
    vtkDataArray* toVectors = toPd->GetVectors();
    if (fromVectors != NULL)
    {
      DataFrom = (vtkFloatArray*)fromVectors;
      DataTo = (vtkFloatArray*)toVectors;
      int activeComponent = -1;
      CopyArrays (DataFrom, DataTo, numToCopy, fromId, activeComponent, myId);
    }
  }

  if ( toPd->GetCopyNormals() )
  {
    vtkDataArray* fromNormals = fromPd->GetNormals();
    vtkDataArray* toNormals = toPd->GetNormals();
    if (fromNormals != NULL)
    {
      DataFrom = fromNormals;
      DataTo = toNormals;
      int activeComponent = -1;
      CopyArrays (DataFrom, DataTo, numToCopy, fromId, activeComponent, myId);
    }
  }

  if ( toPd->GetCopyTCoords() )
  {
    vtkDataArray* fromTCoords = fromPd->GetTCoords();
    vtkDataArray* toTCoords   = toPd->GetTCoords();
    if (fromTCoords != NULL)
    {
      DataFrom = fromTCoords;
      DataTo = toTCoords;
      int activeComponent = -1;
      CopyArrays (DataFrom, DataTo, numToCopy, fromId, activeComponent, myId);
    }
  }

  if ( toPd->GetCopyTensors() )
  {
    vtkDataArray* fromTensors = fromPd->GetTensors();
    vtkDataArray* toTensors = toPd->GetTensors();
    if (fromTensors != NULL)
    {
      DataFrom = fromTensors;
      DataTo = toTensors;
      int activeComponent = -1;
      CopyArrays (DataFrom, DataTo, numToCopy, fromId, activeComponent, myId);
    }
  }

#if 0
  //if ( toPd->GetCopyFieldData() )
  {
    vtkFieldData* fromFieldData = fromPd->GetFieldData();
    vtkFieldData* toFieldData = toPd->GetFieldData();

    vtkDataSetAttributes::FieldList list = fromPd->list;

    if (fromFieldData != NULL)
    {
      //int numArrays=fromFieldData->GetNumberOfArrays();

      //for (int j=0; j<numArrays; j++)
      for (int j=0; j<list.NumberOfFields; j++)
      {
        DataFrom = fromPd->GetArray(list.DSAIndicies[idx][j]);
        DataTo = toPd->GetArray(list.FieldIndicies[j]);
        //DataFrom = fromFieldData->GetArray(j);
        //DataTo = toFieldData->GetArray(j);
        int activeComponent = -1;
        CopyArrays (DataFrom, DataTo, numToCopy, fromId, activeComponent, myId);
      }
    }
  }
#endif
}
//*****************************************************************
// Copy the attribute data from one id to another. Make sure CopyAllocate() has// been invoked before using this method.
void vtkRedistributePoly::CopyCellBlockDataArrays
   (vtkDataSetAttributes* fromPd, vtkDataSetAttributes* toPd,
    const vtkIdType numToCopy, vtkIdType* fromIds, const vtkIdType startCell, 
    const int myId )
//*****************************************************************************
{

  vtkDataArray* DataFrom;
  vtkDataArray* DataTo;

  if ( toPd->GetCopyScalars() )
  {
    vtkDataArray* fromScalars = fromPd->GetScalars();
    vtkDataArray* toScalars = toPd->GetScalars();
    if (fromScalars != NULL)
    {
      DataFrom = fromScalars;
      DataTo = toScalars;
      int numComps = DataFrom->GetNumberOfComponents();
      int activeComponent = 0;
      //int activeComponent = fromScalars->GetActiveComponent();
      if (numComps>1)
        CopyArrays (DataFrom,DataTo, numToCopy, fromIds, activeComponent, myId);
      else
        CopyBlockArrays (DataFrom, DataTo, numToCopy, startCell, myId);
    }
  }

  if ( toPd->GetCopyVectors() )
  {
    vtkDataArray* fromVectors = fromPd->GetVectors();
    vtkDataArray* toVectors = toPd->GetVectors();
    if (fromVectors != NULL)
    {
      DataFrom = (vtkFloatArray*)fromVectors;
      DataTo = (vtkFloatArray*)toVectors;
      CopyBlockArrays (DataFrom, DataTo, numToCopy, startCell, myId);
    }
  }

  if ( toPd->GetCopyNormals() )
  {
    vtkDataArray* fromNormals = fromPd->GetNormals();
    vtkDataArray* toNormals = toPd->GetNormals();
    if (fromNormals != NULL)
    {
      DataFrom = fromNormals;
      DataTo = toNormals;
      CopyBlockArrays (DataFrom, DataTo, numToCopy, startCell, myId);
    }
  }

  if ( toPd->GetCopyTCoords() )
  {
    vtkDataArray* fromTCoords = fromPd->GetTCoords();
    vtkDataArray* toTCoords   = toPd->GetTCoords();
    if (fromTCoords != NULL)
    {
      DataFrom = fromTCoords;
      DataTo = toTCoords;
      CopyBlockArrays (DataFrom, DataTo, numToCopy, startCell, myId);
    }
  }

  if ( toPd->GetCopyTensors() )
  {
    vtkDataArray* fromTensors = fromPd->GetTensors();
    vtkDataArray* toTensors = toPd->GetTensors();
    if (fromTensors != NULL)
    {
      DataFrom = fromTensors;
      DataTo = toTensors;
      CopyBlockArrays (DataFrom, DataTo, numToCopy, startCell, myId);
    }
  }

#if 0
  //if ( toPd->GetCopyFieldData() )
  {
    vtkFieldData* fromFieldData = fromPd->GetFieldData();
    vtkFieldData* toFieldData = toPd->GetFieldData();
    if (fromFieldData != NULL)
    {
      int numArrays=fromFieldData->GetNumberOfArrays();
      for (int j=0; j<numArrays; j++)
      {
        DataFrom = fromFieldData->GetArray(j);
        DataTo = toFieldData->GetArray(j);
        CopyBlockArrays (DataFrom, DataTo, numToCopy, startCell, myId);
      }
    }
  }
#endif
}
//*****************************************************************************
void vtkRedistributePoly::CopyArrays
   (vtkDataArray* DataFrom, vtkDataArray* DataTo, const vtkIdType numToCopy, 
    vtkIdType* fromId, const int activeComponentInp, const int myId)
//*****************************************************************************
{
   char *cArrayFrom, *cArrayTo;
   int *iArrayFrom,  *iArrayTo;
   float *fArrayFrom, *fArrayTo;
   long *lArrayFrom,  *lArrayTo;
   vtkIdType *idArrayFrom,  *idArrayTo;
   unsigned long *ulArrayFrom, *ulArrayTo;
   unsigned char *ucArrayFrom, *ucArrayTo;
   double *dArrayFrom , *dArrayTo;

   vtkIdType i;
   int j;
   int numComps = DataFrom->GetNumberOfComponents();
   int dataType = DataFrom->GetDataType();
   int numCompsToCopy;
   int activeComponent = activeComponentInp;

   if (activeComponent>= 0) 
     // ... scalar case ...
     numCompsToCopy = 1;
   else
   {
     // ... all other cases ...
     numCompsToCopy = numComps;
     activeComponent = 0;
   }

   switch (dataType)
   {
      case VTK_CHAR:
        cArrayFrom = ((vtkCharArray*)DataFrom)->GetPointer(0);
        cArrayTo = ((vtkCharArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            cArrayTo[numCompsToCopy*i+j+activeComponent]=
              cArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];

        break;

      case VTK_UNSIGNED_CHAR:
        ucArrayFrom = ((vtkUnsignedCharArray*)DataFrom)->GetPointer(0);
        ucArrayTo = ((vtkUnsignedCharArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            ucArrayTo[numCompsToCopy*i+j+activeComponent]=
              ucArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];

        break;

      case VTK_INT:
        iArrayFrom = ((vtkIntArray*)DataFrom)->GetPointer(0);
        iArrayTo = ((vtkIntArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            iArrayTo[numCompsToCopy*i+j+activeComponent]=
              iArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];

        break;

      case VTK_UNSIGNED_LONG:
        ulArrayFrom = ((vtkUnsignedLongArray*)DataFrom)->GetPointer(0);
        ulArrayTo = ((vtkUnsignedLongArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            ulArrayTo[numCompsToCopy*i+j+activeComponent]=
              ulArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];
        
        break;

      case VTK_FLOAT:
        fArrayFrom = ((vtkFloatArray*)DataFrom)->GetPointer(0);
        fArrayTo = ((vtkFloatArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            fArrayTo[numCompsToCopy*i+j+activeComponent]=
              fArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];
        
        break;

      case VTK_DOUBLE:
        dArrayFrom = ((vtkDoubleArray*)DataFrom)->GetPointer(0);
        dArrayTo = ((vtkDoubleArray*)DataTo)->GetPointer(0);
        if (!colorProc)
        {
           for (i = 0; i < numToCopy; i++)
             for (j = 0; j < numCompsToCopy; j++)
               dArrayTo[numCompsToCopy*i+j+activeComponent]=
                 dArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];
        }
        else
        {
           for (i = 0; i < numToCopy; i++)
             for (j = 0; j < numCompsToCopy; j++)
               dArrayTo[numCompsToCopy*i+j+activeComponent]= myId;
        }

        break;

      case VTK_LONG:
        lArrayFrom = ((vtkLongArray*)DataFrom)->GetPointer(0);
        lArrayTo = ((vtkLongArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            lArrayTo[numCompsToCopy*i+j+activeComponent]=
              lArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];

        break;
        
      case VTK_ID_TYPE:
        idArrayFrom = ((vtkIdTypeArray*)DataFrom)->GetPointer(0);
        idArrayTo = ((vtkIdTypeArray*)DataTo)->GetPointer(0);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            idArrayTo[numCompsToCopy*i+j+activeComponent]=
              idArrayFrom[numCompsToCopy*fromId[i]+j+activeComponent];

        break;
        
      case VTK_BIT:
        cerr<<"VTK_BIT not allowed for copy"<<endl;
        break;
      case VTK_UNSIGNED_SHORT:
        cerr<<"VTK_UNSIGNED_SHORT not allowed for copy"<<endl;
        break;
      case VTK_SHORT:
        cerr<<"VTK_SHORT not allowed for copy"<<endl;
        break;
      case VTK_UNSIGNED_INT:
        cerr<<"VTK_UNSIGNED_INT not allowed for copy"<<endl;
        break;
      default:
        cerr<<"datatype = "<<dataType<<" not allowed for copy"<<endl;
   }
}
//----------------------------------------------------------------------
//*****************************************************************************
void vtkRedistributePoly::CopyBlockArrays
   (vtkDataArray* DataFrom, vtkDataArray* DataTo, const vtkIdType numToCopy, 
    const vtkIdType startCell, const int myId)
//*****************************************************************************
{
   char *cArrayTo, *cArrayFrom;
   int *iArrayTo, *iArrayFrom;
   float *fArrayTo, *fArrayFrom;
   long *lArrayTo, *lArrayFrom;
   vtkIdType *idArrayTo, *idArrayFrom;
   unsigned long *ulArrayTo, *ulArrayFrom;
   unsigned char *ucArrayTo, *ucArrayFrom;
   double *dArrayTo, *dArrayFrom;

   int numComps = DataFrom->GetNumberOfComponents();
   int dataType = DataFrom->GetDataType();

   vtkIdType  start = numComps*startCell;
   vtkIdType  size = numToCopy*numComps;
   vtkIdType  stop = start + size;

   vtkIdType i;

   switch (dataType)
   {
      case VTK_CHAR:
        cArrayFrom = ((vtkCharArray*)DataFrom)->GetPointer(0);
        cArrayTo = ((vtkCharArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) cArrayTo[i] = cArrayFrom[i];
        break;

      case VTK_UNSIGNED_CHAR:
        ucArrayFrom = ((vtkUnsignedCharArray*)DataFrom)->GetPointer(0);
        ucArrayTo = ((vtkUnsignedCharArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) ucArrayTo[i] = ucArrayFrom[i];
        break;

      case VTK_INT:
        iArrayFrom = ((vtkIntArray*)DataFrom)->GetPointer(0);
        iArrayTo = ((vtkIntArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) iArrayTo[i] = iArrayFrom[i];
        break;

      case VTK_UNSIGNED_LONG:
        ulArrayFrom = ((vtkUnsignedLongArray*)DataFrom)->GetPointer(0);
        ulArrayTo = ((vtkUnsignedLongArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) ulArrayTo[i] = ulArrayFrom[i];
        break;

      case VTK_FLOAT:
        fArrayFrom = ((vtkFloatArray*)DataFrom)->GetPointer(0);
        fArrayTo = ((vtkFloatArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) fArrayTo[i] = fArrayFrom[i];
        break;

      case VTK_DOUBLE:
        dArrayFrom = ((vtkDoubleArray*)DataFrom)->GetPointer(0);
        dArrayTo = ((vtkDoubleArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) dArrayTo[i] = dArrayFrom[i];
        if (!colorProc)
          for (i=start; i<stop; i++) dArrayTo[i] = dArrayFrom[i];
        else
          for (i=start; i<stop; i++) dArrayTo[i] = myId;
        break;

      case VTK_LONG:
        lArrayFrom = ((vtkLongArray*)DataFrom)->GetPointer(0);
        lArrayTo = ((vtkLongArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) lArrayTo[i] = lArrayFrom[i];
        break;
        
      case VTK_ID_TYPE:
        idArrayFrom = ((vtkIdTypeArray*)DataFrom)->GetPointer(0);
        idArrayTo = ((vtkIdTypeArray*)DataTo)->GetPointer(0);
        for (i=start; i<stop; i++) idArrayTo[i] = idArrayFrom[i];
        break;
        
      case VTK_BIT:
        cerr<<"VTK_BIT not allowed for copy"<<endl;
        break;
      case VTK_UNSIGNED_SHORT:
        cerr<<"VTK_UNSIGNED_SHORT not allowed for copy"<<endl;
        break;
      case VTK_SHORT:
        cerr<<"VTK_SHORT not allowed for copy"<<endl;
        break;
      case VTK_UNSIGNED_INT:
        cerr<<"VTK_UNSIGNED_INT not allowed for copy"<<endl;
        break;
      default:
        cerr<<"datatype = "<<dataType<<" not allowed for copy"<<endl;
   }
}
//*****************************************************************
//*****************************************************************
void vtkRedistributePoly::CopyCells (const vtkIdType numCells, 
                                     vtkPolyData* input, vtkPolyData* output, 
                                     vtkIdType* keepCellList)

//*****************************************************************
{
  // ... Copy initial subset of cells and points from input to output.  
  //     This assumes that the cells will be copied from the beginning 
  //     of the list. ... 


  int myId = this->Controller->GetLocalProcessId();
  vtkIdType cellId,i;

  // ... Copy cell data attribute data (Scalars, Vectors, etc.)...

  vtkIdType* fromIds; 
  if (keepCellList == NULL)
  {
     fromIds = new vtkIdType [numCells];
     for (cellId = 0; cellId <numCells; cellId++) fromIds[cellId] = cellId;
  }
  else
  {
     fromIds = keepCellList;
  }

  vtkCellData* inputCellData = input->GetCellData();
  vtkCellData* outputCellData = output->GetCellData();

  if (keepCellList == NULL)
  {
     vtkIdType startCell = 0;
     CopyCellBlockDataArrays (inputCellData, outputCellData, numCells, fromIds, 
                              startCell, myId);
     delete [] fromIds;
  }
  else
  {
     CopyDataArrays (inputCellData, outputCellData, numCells, fromIds, myId);
  }


#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==-7)cerr<<"1st copy data arrays time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif


  vtkPoints *outputPoints = output->GetPoints();
  vtkFloatArray* outputArray = (vtkFloatArray*)(outputPoints->GetData());
  vtkPoints *inputPoints = input->GetPoints();
  vtkFloatArray* inputArray = NULL;
  if (inputPoints != NULL)
    inputArray = (vtkFloatArray*)(inputPoints->GetData());

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==-7)cerr<<"alloc  pts time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  float* outputArrayData = outputArray->GetPointer(0);
  float* inputArrayData = NULL;
  if (inputArray != NULL) inputArrayData = inputArray->GetPointer(0);


  // ... Allocate maximum possible number of points (use total from
  //     all of input) ... 

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* fromPtIds = new vtkIdType[numPointsMax];

  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i=0; i<numPointsMax;i++) usedIds[i]=-1;


  // ... Copy point Id's for all the points in the cell. ...

  vtkIdType pointIncr = 0;
  vtkIdType pointId; 
  vtkCellArray *inputPolys = input->GetPolys();
  vtkCellArray *outputPolys = output->GetPolys();
  vtkIdType* inPtr = inputPolys->GetPointer();
  vtkIdType* ptr = outputPolys->GetPointer();
  vtkIdType npts;

  if (keepCellList == NULL)
  {
     for (cellId = 0; cellId < numCells; cellId++)
     {
       // ... set output number of points to input number of points ...
       npts=*inPtr++;
       *ptr++ = npts;
       for (i = 0; i < npts; i++)
       {
   	  pointId = *inPtr++;
          if (usedIds[pointId] == -1)
          {
             vtkIdType newPt = pointIncr;
             *ptr++ = newPt;
             usedIds[pointId] = newPt;
             fromPtIds[pointIncr] = pointId;
             pointIncr++;
          }
          else
          {
             // ... use new point id ...
             *ptr++ = usedIds[pointId];
          }
       }
     }
  }
  else
  {
     vtkIdType prevCellId = 0;
     for (vtkIdType id = 0; id < numCells; id++)
     {
       cellId = keepCellList[id];
       //cerr<<"myId="<<myId<<",cellId="<<cellId<<endl;
       for (i=prevCellId; i<cellId; i++)
       {
          npts=*inPtr++;
          inPtr += npts;
       }
       prevCellId = cellId+1;

       // ... set output number of points to input number of points ...
       npts=*inPtr++;
       *ptr++ = npts;
       for (i = 0; i < npts; i++)
       {
   	  pointId = *inPtr++;
          if (usedIds[pointId] == -1)
          {
             vtkIdType newPt = pointIncr;
             *ptr++ = newPt;
             usedIds[pointId] = newPt;
             fromPtIds[pointIncr] = pointId;
             pointIncr++;
          }
          else
          {
             // ... use new point id ...
             *ptr++ = usedIds[pointId];
          }
       }
     }
  }

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==-7)cerr<<"copy pt ids time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif


  // ... Copy cell points. ...
  vtkIdType inLoc, outLoc;
  vtkIdType numPoints = pointIncr;
  int j;
  for (i=0; i<numPoints; i++)
  {
     inLoc = fromPtIds[i]*3;
     outLoc = i*3;
     for (j=0;j<3;j++) outputArrayData[outLoc+j] = inputArrayData[inLoc+j];
  }

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==-7)cerr<<"copy pts time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

  vtkPointData* inputPointData = input->GetPointData();
  vtkPointData* outputPointData = output->GetPointData();

  CopyDataArrays (inputPointData, outputPointData, numPoints, fromPtIds, myId );
  delete [] fromPtIds;

#if (DO_TIMING==1) 
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==-7)cerr<<"setting time = "<<timerInfo8.time<<endl;
  timerInfo8.timer->StartTimer();
#endif

}
//*****************************************************************
void vtkRedistributePoly::SendCellSizes 
   (const vtkIdType startCell, const vtkIdType stopCell, vtkPolyData* input, 
    const int sendTo, vtkIdType& numPoints, vtkIdType& ptcntr, 
    vtkIdType* sendCellList)

//*****************************************************************
{
  // ... send cells and point sizes without sending data ...

  vtkIdType cellId,i;
  vtkIdType numCells = stopCell-startCell+1;

  // ... Allocate maximum possible number of points (use total from
  //     all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* usedIds = new vtkIdType [numPointsMax];
  for (i=0; i<numPointsMax;i++) usedIds[i]=-1;


  // ... send point Id's for all the points in the cell. ...


  vtkIdType pointIncr = 0;
  vtkIdType pointId; 
  vtkCellArray *inputPolys = input->GetPolys();
  vtkIdType* inPtr = inputPolys->GetPointer();
  vtkIdType npts;

  ptcntr = 0;

  if (sendCellList == NULL)
  {
     // ... send cells in a block ...
     for (cellId = 0; cellId < startCell; cellId++)
     {
       // ... increment pointers to get to correct starting point ...
       npts=*inPtr++;
       inPtr+=npts;
     }
   
     for (cellId = startCell; cellId <= stopCell; cellId++)
     {
       // ... set output number of points to input number of points ...
       npts=*inPtr++;
       ptcntr++;
       for (i = 0; i < npts; i++)
       {
           pointId = *inPtr++;
           if (usedIds[pointId] == -1) usedIds[pointId] = pointIncr++;
           ptcntr++;
       }
     }
  }
  else
  {
     // ... there is a specific list of cells to send ...

     vtkIdType prevCellId = 0;

     for (vtkIdType id = 0; id < numCells; id++)
     {
       
       cellId = sendCellList[id];
       for (i = prevCellId; i<cellId ; i++)
       {
         // ... increment pointers to get to correct starting point ...
         npts=*inPtr++;
         inPtr+=npts;
       }
       prevCellId = cellId+1;

       // ... set output number of points to input number of points ...

       npts=*inPtr++;
       ptcntr++;

       for (i = 0; i < npts; i++)
       {
           pointId = *inPtr++;
           if (usedIds[pointId] == -1) usedIds[pointId] = pointIncr++;
           ptcntr++;
       }
     }
  }

  // ... send sizes first (must be in this order to allocate for receive)...

  this->Controller->Send((vtkIdType*)&ptcntr, 1, sendTo, VTK_CELL_CNT_TAG);

  numPoints = pointIncr;
  this->Controller->Send((vtkIdType*)&numPoints, 1, sendTo,
                          VTK_POINTS_SIZE_TAG);

}
//*****************************************************************
void vtkRedistributePoly::SendCells 
   (const vtkIdType startCell, const vtkIdType stopCell,
    vtkPolyData* input, vtkPolyData* output, const int sendTo, 
    vtkIdType& numPoints, vtkIdType& cellArraySize, vtkIdType* sendCellList)

//*****************************************************************
{
  // ... send cells, points and associated data from cells in
  //     specified region ...

  vtkIdType cellId,i;
  vtkIdType numCells = stopCell-startCell+1;

  // ... Allocate maximum possible number of points (use total from
  //     all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* fromPtIds = new vtkIdType[numPointsMax];

  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i=0; i<numPointsMax;i++) usedIds[i]=-1;


  // ... send point Id's for all the points in the cell. ...

  vtkIdType* ptr = new vtkIdType[cellArraySize];
  vtkIdType ptcntr = 0;
  vtkIdType* ptrsav = ptr;

  vtkIdType pointIncr = 0;
  vtkIdType pointId; // DOES THIS NEED TO BE A LONG?
  vtkCellArray *inputPolys = input->GetPolys();
  vtkIdType* inPtr = inputPolys->GetPointer();
  vtkIdType npts;

  if (sendCellList == NULL)
  {
     // ... send cells in a block ...
     for (cellId = 0; cellId < startCell; cellId++)
     {
       // ... increment pointers to get to correct starting point ...
       npts=*inPtr++;
       inPtr+=npts;
     }
   
     for (cellId = startCell; cellId <= stopCell; cellId++)
     {
       // ... set output number of points to input number of points ...
       npts=*inPtr++;
       *ptr++ = npts;
       ptcntr++;
       for (i = 0; i < npts; i++)
       {
           pointId = *inPtr++;
           if (usedIds[pointId] == -1)
           {
              vtkIdType newPt = pointIncr;
              *ptr++ = newPt;
              ptcntr++;
              usedIds[pointId] = newPt;
              fromPtIds[pointIncr] = pointId;
              pointIncr++;
           }
           else
           {
              // ... use new point id ...
              *ptr++ = usedIds[pointId];
              ptcntr++;
           }
       }
     }
  }
  else
  {
     // ... there is a specific list of cells to send ...

     vtkIdType prevCellId = 0;

     for (vtkIdType  id = 0; id < numCells; id++)
     {
       
       cellId = sendCellList[id];
       for (i = prevCellId; i<cellId ; i++)
       {
         // ... increment pointers to get to correct starting point ...
         npts=*inPtr++;
         inPtr+=npts;
       }
       prevCellId = cellId+1;

       // ... set output number of points to input number of points ...

       npts=*inPtr++;
       *ptr++ = npts;
       ptcntr++;

       for (i = 0; i < npts; i++)
       {
           pointId = *inPtr++;
           if (usedIds[pointId] == -1)
           {
              vtkIdType newPt = pointIncr;
              *ptr++ = newPt;
              ptcntr++;
              usedIds[pointId] = newPt;
              fromPtIds[pointIncr] = pointId;
              pointIncr++;
           }
           else
           {
              // ... use new point id ...
              *ptr++ = usedIds[pointId];
              ptcntr++;
           }
       }
     }
  }
  if (numPoints != pointIncr) cerr<<"ERROR: numPoints="<<numPoints
    <<", pointIncr="<<pointIncr<<", should be equal"<<endl;;

  delete [] usedIds;



 // ... send cell data attribute data (Scalars, Vectors, etc.)...

  vtkIdType* fromIds;
  vtkIdType cnt = 0;
  if (sendCellList == NULL)
  {
    fromIds = new vtkIdType[numCells];
    for (cellId = startCell; cellId <=stopCell; cellId++) 
      fromIds[cnt++]= cellId;
  }
  else
   fromIds = sendCellList;


  // ... output needed for flags only (assumes flags are the same on
  //     all processors) ...
  vtkCellData* inputCellData = input->GetCellData();
  vtkCellData* outputCellData = output->GetCellData();
  int typetag;  //(typetag = 0 for cells, =1 for points)
  if (sendCellList == NULL)
  {
     SendCellBlockDataArrays (inputCellData, outputCellData, numCells, 
                              sendTo, fromIds, startCell );
     delete [] fromIds; // this array was allocated above in this case
  }
  else
  {
     typetag = 0; //(typetag = 0 for cells)
     SendDataArrays (inputCellData, outputCellData, numCells, sendTo, 
                     fromIds, typetag);
  }


  // ... Send points Id's in cells now ...

  this->Controller->Send(ptrsav, ptcntr, sendTo, VTK_CELL_TAG);


  // ... Copy cell points. ...

  vtkPoints *inputPoints = input->GetPoints();
  vtkFloatArray* inputArray = (vtkFloatArray*)(inputPoints->GetData());
  float* inputArrayData = inputArray->GetPointer(0);
  float* outputArrayData = new float[3*numPoints];

  int j;
  vtkIdType inLoc, outLoc;
  for (i=0; i<numPoints; i++)
  {
     inLoc = fromPtIds[i]*3;
     outLoc = i*3;
     for (j=0;j<3;j++) outputArrayData[outLoc+j] = inputArrayData[inLoc+j];
  }


  // ... Send points now ...

  this->Controller->Send(outputArrayData, 3*numPoints, sendTo,
                         VTK_POINTS_TAG);


  // ... use output for flags only to avoid unneccessary sends ...
  vtkPointData* inputPointData = input->GetPointData();
  vtkPointData* outputPointData = output->GetPointData();
  typetag = 1; //(typetag = 0 for cells, =1 for points)
  SendDataArrays (inputPointData, outputPointData, numPoints, sendTo,
                  fromPtIds, typetag);
  delete [] fromPtIds;

}
//****************************************************************************8
void vtkRedistributePoly::ReceiveCells
   (const vtkIdType startCell, const vtkIdType stopCell,
    vtkPolyData* output, const int recFrom,
    const vtkIdType prevCellptCntr,const vtkIdType cellptCntr,
    const vtkIdType prevNumPoints, const vtkIdType numPoints)

//*****************************************************************
{
  // ... send cells, points and associated data from cells in
  //     specified region ...

  vtkIdType cellId,i;
  vtkIdType numCells = stopCell-startCell+1;


  // ... receive cell data attribute data (Scalars, Vectors, etc.)...

  vtkIdType* toIds = new vtkIdType[numCells];
  vtkIdType cnt = 0;
  for (cellId = startCell; cellId <=stopCell; cellId++) toIds[cnt++]= cellId;

  // ... output needed for flags only (assumes flags are the same on
  //     all processors) ...
  vtkCellData* outputCellData = output->GetCellData();
  int typetag = 0; //(typetag = 0 for cells, =1 for points)
  ReceiveDataArrays (outputCellData, numCells, recFrom, toIds, typetag);
  delete [] toIds;


  // ... receive point Id's for all the points in the cell. ...

  vtkCellArray *outputPolys = output->GetPolys();
  vtkIdType* outPtr = outputPolys->GetPointer();
  outPtr+= prevCellptCntr;

  this->Controller->Receive((vtkIdType*)outPtr, cellptCntr, recFrom, VTK_CELL_TAG);

  // ... Fix pointId's (need to have offset added to represent correct 
  //   location ...
  for (cellId = startCell; cellId <=stopCell; cellId++) 
  {
     vtkIdType npts=*outPtr++;
     for (i = 0; i < npts; i++)
     {
        *outPtr+=prevNumPoints;
        outPtr++;
     }
  }
  

  // ... Receive points now ...

  vtkPoints *outputPoints = output->GetPoints();
  vtkFloatArray* outputArray = (vtkFloatArray*)(outputPoints->GetData());
  float* outputArrayData = outputArray->GetPointer(0);

  this->Controller->Receive(&outputArrayData[prevNumPoints*3], 3*numPoints,
                            recFrom, VTK_POINTS_TAG);


  // ... receive point attribute data ...
  vtkIdType* toPtIds = new vtkIdType[numPoints];
  for (i=0; i<numPoints; i++) toPtIds[i] = prevNumPoints+i;

  vtkPointData* outputPointData = output->GetPointData();
  typetag = 1; //(typetag = 0 for cells, =1 for points)
  ReceiveDataArrays (outputPointData, numPoints, recFrom, toPtIds, typetag);
  delete [] toPtIds;

}
//*******************************************************************
// Allocate space for the attribute data expected from all id's.
void vtkRedistributePoly::AllocateDataArrays
   (vtkDataSetAttributes* toPd, vtkIdType* numToCopy, const int cntRec,
    int*, const vtkIdType numToCopyOnProc)
{
  vtkIdType numToCopyTotal = numToCopyOnProc;
  int id;
  for (id=0;id<cntRec;id++)
    {
    numToCopyTotal += numToCopy[id];
    }
   

  // ... Use WritePointer to allocate memory because it copies existing 
  //   data and only allocates if necessary. ...

  vtkDataArray* Data;

  if ( toPd->GetCopyScalars() )
    {
    vtkDataArray* toScalars = toPd->GetScalars();
    if (toScalars)
      {
      Data = toScalars;
      AllocateArrays (Data, numToCopyTotal );
      }
    }

  if ( toPd->GetCopyVectors() )
    {
    vtkDataArray* toVectors = toPd->GetVectors();
    if (toVectors)
      {
      Data = toVectors;
      AllocateArrays (Data, numToCopyTotal );
      }
    }

  if ( toPd->GetCopyNormals() )
    {
    vtkDataArray* toNormals = toPd->GetNormals();
    if (toNormals)
      {
      Data = toNormals;
      AllocateArrays (Data, numToCopyTotal );
      }
    }

  if ( toPd->GetCopyTCoords() )
    {
    vtkDataArray* toTCoords = toPd->GetTCoords();
    if (toTCoords)
      {
      Data = toTCoords;
      AllocateArrays (Data, numToCopyTotal);
      }
    }

  if ( toPd->GetCopyTensors() )
    {
    vtkDataArray* toTensors = toPd->GetTensors();
    if (toTensors)
      {
      Data = toTensors;
      AllocateArrays (Data, numToCopyTotal);
      }
    }

#if 0
  //if ( toPd->GetCopyFieldData() )
  {
  vtkFieldData* toFieldData = toPd->GetFieldData();
  if (toFieldData != NULL)
    {
    int numArrays=toFieldData->GetNumberOfArrays();
    for (int j=0; j<numArrays; j++)
      {
      Data = toFieldData->GetArray(j);
      AllocateArrays (Data, numToCopyTotal);

      }
    }
  }
#endif

}
//************************************************************************
void vtkRedistributePoly::AllocateArrays
   (vtkDataArray* Data, const vtkIdType numToCopyTotal )
//************************************************************************
{
   int dataType = Data->GetDataType();
   int numComp = Data->GetNumberOfComponents();

   if (numToCopyTotal >0)
   {
   switch (dataType)
   {
      case VTK_CHAR:

        if (((vtkCharArray*)Data)-> 
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;

      case VTK_UNSIGNED_CHAR:

        if (((vtkUnsignedCharArray*)Data)-> 
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;

      case VTK_INT:

        if (((vtkIntArray*)Data)->
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;

      case VTK_UNSIGNED_LONG:

        if (((vtkUnsignedLongArray*)Data)->
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;

      case VTK_FLOAT:

        if (((vtkFloatArray*)Data)->
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;

      case VTK_DOUBLE:

        if (((vtkDoubleArray*)Data)->
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;

      case VTK_LONG:

        if (((vtkLongArray*)Data)->
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;
        
      case VTK_ID_TYPE:

        if (((vtkIdTypeArray*)Data)->
            WritePointer(0,numToCopyTotal*numComp) ==0)
            cerr<<"Error: can't alloc mem for data array"<<endl;

        break;
        
      case VTK_BIT:
        cerr<<"VTK_BIT not allowed for Data Arrays"<<endl;
        break;
      case VTK_UNSIGNED_SHORT:
        cerr<<"VTK_UNSIGNED_SHORT not allowed for Data Arrays"<<endl;
        break;
      case VTK_SHORT:
        cerr<<"VTK_SHORT not allowed for Data Arrays"<<endl;
        break;
      case VTK_UNSIGNED_INT:
        cerr<<"VTK_UNSIGNED_INT not allowed for Data Arrays"<<endl;
        break;
      default:
        cerr<<"datatype = "<<dataType<<" not allowed for Data Arrays"
            <<endl;
   } // end of switch
   } // end of if numToCopyTotal>0
}
//----------------------------------------------------------------------
//*****************************************************************
void vtkRedistributePoly::FindMemReq
  (const vtkIdType origNumCells, vtkPolyData* input, vtkIdType& numPoints,
    vtkIdType& numCellPts)

//*****************************************************************
{
  // ... count number of cellpoints, corresponding points and number
  //   of cells ...
  vtkIdType cellId,i;

  // ... Allocate maximum possible number of points (use total from
  //     all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i=0; i<numPointsMax;i++) usedIds[i]=-1;


  // ... count point Id's for all the points in the cell
  //     and number of points that will be stored ...

  vtkIdType pointId; 

  vtkCellArray *inputPolys = input->GetPolys();
  vtkIdType* inPtr = inputPolys->GetPointer();

  numCellPts = 0;
  numPoints = 0;
  for (cellId = 0; cellId < origNumCells; cellId++)
  {
    vtkIdType npts=*inPtr++;
    numCellPts++;
    numCellPts+=npts;
    for (i = 0; i < npts; i++)
    {
        pointId = *inPtr++;
        if (usedIds[pointId] == -1)
        {
           vtkIdType newPt = numPoints;
           usedIds[pointId] = newPt;
           numPoints++;
        }
    }
  }

  delete [] usedIds;
}

//*****************************************************************
//*****************************************************************
// Copy the attribute data from one id to another. Make sure CopyAllocate() has// been invoked before using this method.
void vtkRedistributePoly::SendDataArrays
   (vtkDataSetAttributes* fromPd, vtkDataSetAttributes* toPd,
    const vtkIdType numToCopy, const int sendTo, vtkIdType* fromId, 
    const int typetag)
{

  vtkDataArray* Data;

  if ( toPd->GetCopyScalars() )
  {
    vtkDataArray* fromScalars = fromPd->GetScalars();
    if (fromScalars != NULL)
    {
      Data = fromScalars;
      int activeComponent = 0;
      //int activeComponent = fromScalars->GetActiveComponent();
      int sendTag = VTK_SCALARS_TAG+typetag;
      SendArrays (Data, numToCopy, sendTo, fromId, activeComponent, sendTag);
    }
  }

  if ( toPd->GetCopyVectors() )
  {
    vtkDataArray* fromVectors = fromPd->GetVectors();
    if (fromVectors != NULL)
    {
      Data = (vtkFloatArray*)fromVectors;
      int activeComponent = -1;
      int sendTag = VTK_VECTORS_TAG+typetag;
      SendArrays (Data, numToCopy, sendTo, fromId, activeComponent, sendTag);
    }
  }

  if ( toPd->GetCopyNormals() )
  {
    vtkDataArray* fromNormals = fromPd->GetNormals();
    if (fromNormals != NULL)
    {
      Data = fromNormals;
      int activeComponent = -1;
      int sendTag = VTK_NORMALS_TAG+typetag;
      SendArrays (Data, numToCopy, sendTo, fromId, activeComponent, sendTag);
    }
  }

  if ( toPd->GetCopyTCoords() )
  {
    vtkDataArray* fromTCoords = fromPd->GetTCoords();
    //vtkDataArray* toTCoords   = toPd->GetTCoords();
    if (fromTCoords != NULL)
    {
      Data = fromTCoords;
      int activeComponent = -1;
      int sendTag = VTK_TCOORDS_TAG+typetag;
      SendArrays (Data, numToCopy, sendTo, fromId, activeComponent, sendTag);
    }
  }

  if ( toPd->GetCopyTensors() )
  {
    vtkDataArray* fromTensors = fromPd->GetTensors();
    if (fromTensors != NULL)
    {
      Data = fromTensors;
      int activeComponent = -1;
      int sendTag = VTK_TENSOR_TAG+typetag;
      SendArrays (Data, numToCopy, sendTo, fromId, activeComponent, sendTag);
    }
  }

#if 0
  //if ( toPd->GetCopyFieldData() )
  {
    vtkFieldData* fromFieldData = fromPd->GetFieldData();
    if (fromFieldData != NULL)
    {
      int numArrays=fromFieldData->GetNumberOfArrays();
      for (int j=0; j<numArrays; j++)
      {
        Data = fromFieldData->GetArray(j);
        int activeComponent = -1;
        int sendTag = VTK_FIELDDATA_TAG+typetag+j*100;
        SendArrays (Data, numToCopy, sendTo, fromId, activeComponent, sendTag);
      }
    }
  }
#endif
}
//*****************************************************************
// Copy the attribute data from one id to another. Make sure CopyAllocate() has// been invoked before using this method.
void vtkRedistributePoly::SendCellBlockDataArrays
   (vtkDataSetAttributes* fromPd, vtkDataSetAttributes* toPd,
    const vtkIdType numToCopy, const int sendTo, vtkIdType* fromIds, 
    const vtkIdType startCell )
//*****************************************************************************
{

  vtkDataArray* Data;

  if ( toPd->GetCopyScalars() )
    {
    vtkDataArray* fromScalars = fromPd->GetScalars();
    if (fromScalars != NULL)
      {
      Data = fromScalars;
      int numComps = Data->GetNumberOfComponents();
      int activeComponent = 0;
      //int activeComponent = fromScalars->GetActiveComponent();
      int sendTag = VTK_SCALARS_TAG;
      if (numComps>1)
        SendArrays (Data, numToCopy, sendTo, fromIds, activeComponent, 
                    sendTag);
      else
        SendBlockArrays (Data, numToCopy, sendTo, startCell, sendTag);
      }
    }

  if ( toPd->GetCopyVectors() )
    {
    vtkDataArray* fromVectors = fromPd->GetVectors();
    if (fromVectors != NULL)
      {
      Data = (vtkFloatArray*)fromVectors;
      int sendTag = VTK_VECTORS_TAG;
      SendBlockArrays (Data, numToCopy, sendTo, startCell, sendTag);
      }
    }

  if ( toPd->GetCopyNormals() )
    {
    vtkDataArray* fromNormals = fromPd->GetNormals();
    if (fromNormals != NULL)
      {
      Data = fromNormals;
      int sendTag = VTK_NORMALS_TAG;
      SendBlockArrays (Data, numToCopy, sendTo, startCell, sendTag);
      }
    }

  if ( toPd->GetCopyTCoords() )
    {
    vtkDataArray* fromTCoords = fromPd->GetTCoords();
    //vtkDataArray* toTCoords   = toPd->GetTCoords();
    if (fromTCoords != NULL)
      {
      Data = fromTCoords;
      int sendTag = VTK_TCOORDS_TAG;
      SendBlockArrays (Data, numToCopy, sendTo, startCell, sendTag);
      }
    }

  if ( toPd->GetCopyTensors() )
    {
    vtkDataArray* fromTensors = fromPd->GetTensors();
    if (fromTensors != NULL)
      {
      Data = fromTensors;
      int sendTag = VTK_TENSOR_TAG;
      SendBlockArrays (Data, numToCopy, sendTo, startCell, sendTag);
      }
    }

#if 0
  //if ( toPd->GetCopyFieldData() )
  {
  vtkFieldData* fromFieldData = fromPd->GetFieldData();
  if (fromFieldData != NULL)
    {
    int numArrays=fromFieldData->GetNumberOfArrays();
    for (int j=0; j<numArrays; j++)
      {
      Data = fromFieldData->GetArray(j);
      int sendTag = VTK_FIELDDATA_TAG+j*100;
      SendBlockArrays (Data, numToCopy, sendTo, startCell, sendTag);
      }
    }
  }
#endif
}
//*****************************************************************************
void vtkRedistributePoly::SendArrays
   (vtkDataArray* Data, const vtkIdType numToCopy, const int sendTo, 
    vtkIdType* fromId, const int activeComponentInp, const int sendTag)
//*****************************************************************************
{
   char* sc;
   char *cArray;
   int *iArray, *si;
   float *fArray, *sf;
   long *lArray, *sl;
   vtkIdType *idArray, *sid;
   unsigned long *ulArray, *sul;
   unsigned char *ucArray, *suc;
   double *dArray, *sd;
   int dataSize;

   vtkIdType i;
   int j;
   int numComps = Data->GetNumberOfComponents();
   int dataType = Data->GetDataType();
   int numCompsToCopy;
   int activeComponent = activeComponentInp;

   if (activeComponent>= 0) 
     // ... scalar case ...
     numCompsToCopy = 1;
   else
   {
     // ... all other cases ...
     numCompsToCopy = numComps;
     activeComponent = 0;
   }

   switch (dataType)
   {
      case VTK_CHAR:
        cArray = ((vtkCharArray*)Data)->GetPointer(0);
        sc = new char[numToCopy*numCompsToCopy];
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            sc[numCompsToCopy*i+j] = 
              cArray[numCompsToCopy*fromId[i]+j+activeComponent];

        this->Controller->
          Send(sc, numToCopy*numCompsToCopy, sendTo, sendTag);
        delete [] sc;
        break;

      case VTK_UNSIGNED_CHAR:
        ucArray = ((vtkUnsignedCharArray*)Data)->GetPointer(0);
        suc = new unsigned char[numToCopy*numCompsToCopy];
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            suc[numCompsToCopy*i+j] = 
              ucArray[numCompsToCopy*fromId[i]+j+activeComponent];

        this->Controller->
          Send((char*)suc, numToCopy*numCompsToCopy, sendTo, sendTag);
        delete [] suc;
        break;

      case VTK_INT:
        iArray = ((vtkIntArray*)Data)->GetPointer(0);
        si = new int[numToCopy*numCompsToCopy];
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            si[numCompsToCopy*i+j] = 
              iArray[numCompsToCopy*fromId[i]+j+activeComponent];

        this->Controller->
          Send(si, numToCopy*numCompsToCopy, sendTo, sendTag);
        delete [] si;
        break;

      case VTK_UNSIGNED_LONG:
        ulArray = 
          ((vtkUnsignedLongArray*)Data)->GetPointer(0);
        sul = new unsigned long [numToCopy*numCompsToCopy];
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            sul[numCompsToCopy*i+j] = 
              ulArray[numCompsToCopy*fromId[i]+j+activeComponent];
        
        this->Controller->
          Send(sul, numToCopy*numCompsToCopy, sendTo, sendTag);
        delete [] sul;
        break;

      case VTK_FLOAT:
        fArray = ((vtkFloatArray*)Data)->GetPointer(0);
        sf = new float[numToCopy*numCompsToCopy];
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            sf[numCompsToCopy*i+j] = 
              fArray[numCompsToCopy*fromId[i]+j+activeComponent];
        
        this->Controller->
          Send(sf, numToCopy*numCompsToCopy, sendTo, sendTag);
        delete [] sf;
        break;

      case VTK_DOUBLE:
        dArray = ((vtkDoubleArray*)Data)->GetPointer(0);
        dataSize = sizeof(double);
        sc = (char*)new char[numToCopy*dataSize*numCompsToCopy];
        sd = (double*)sc;
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            sd[numCompsToCopy*i+j] = 
              dArray[numCompsToCopy*fromId[i]+j+activeComponent];

        this->Controller->
          Send(sc, numToCopy*numCompsToCopy*dataSize, sendTo, sendTag);
        delete [] sc;
        break;

      case VTK_LONG:
        lArray = ((vtkLongArray*)Data)->GetPointer(0);
        dataSize = sizeof(long);
        sc = (char*)new long[numToCopy*dataSize*numCompsToCopy];
        sl = (long*)sc;
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            sl[numCompsToCopy*i+j] = 
              lArray[numCompsToCopy*fromId[i]+j+activeComponent];

        this->Controller->
          Send(sc, numToCopy*numCompsToCopy*dataSize, sendTo, sendTag);
        delete [] sc;
        break;
        
      case VTK_ID_TYPE:
        idArray = ((vtkIdTypeArray*)Data)->GetPointer(0);
        dataSize = sizeof(vtkIdType);
        sc = (char*)new vtkIdType[numToCopy*dataSize*numCompsToCopy];
        sid = (vtkIdType*)sc;
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            sid[numCompsToCopy*i+j] = 
              idArray[numCompsToCopy*fromId[i]+j+activeComponent];

        this->Controller->
          Send(sc, numToCopy*numCompsToCopy*dataSize, sendTo, sendTag);
        delete [] sc;
        break;
        
      case VTK_BIT:
        cerr<<"VTK_BIT not allowed for send"<<endl;
        break;
      case VTK_UNSIGNED_SHORT:
        cerr<<"VTK_UNSIGNED_SHORT not allowed for send"<<endl;
        break;
      case VTK_SHORT:
        cerr<<"VTK_SHORT not allowed for send"<<endl;
        break;
      case VTK_UNSIGNED_INT:
        cerr<<"VTK_UNSIGNED_INT not allowed for send"<<endl;
        break;
      default:
        cerr<<"datatype = "<<dataType<<" not allowed for send"<<endl;
   }
}
//----------------------------------------------------------------------
//*****************************************************************************
void vtkRedistributePoly::SendBlockArrays
   (vtkDataArray* Data, const vtkIdType numToCopy, const int sendTo, 
    const vtkIdType startCell, const int sendTag)
//*****************************************************************************
{
   char *cArray;
   int *iArray;
   float *fArray;
   long *lArray;
   vtkIdType *idArray;
   unsigned long *ulArray;
   unsigned char *ucArray;
   double *dArray;
   int dataSize;

   int numComps = Data->GetNumberOfComponents();
   int dataType = Data->GetDataType();

   vtkIdType start = numComps*startCell;
   vtkIdType size = numToCopy*numComps;

   switch (dataType)
   {
      case VTK_CHAR:
        cArray = ((vtkCharArray*)Data)->GetPointer(0);
        this->Controller->
          Send((char*)&cArray[start], size, sendTo, sendTag);
        break;

      case VTK_UNSIGNED_CHAR:
        ucArray = ((vtkUnsignedCharArray*)Data)->GetPointer(0);
        this->Controller->
          Send((char*)&ucArray[start], size, sendTo, sendTag);
        break;

      case VTK_INT:
        iArray = ((vtkIntArray*)Data)->GetPointer(0);
        this->Controller->
          Send((int*)&iArray[start], size, sendTo, sendTag);
        break;

      case VTK_UNSIGNED_LONG:
        ulArray = ((vtkUnsignedLongArray*)Data)->GetPointer(0);
        this->Controller->
          Send((unsigned long*)&ulArray[start], size, sendTo, sendTag);
        break;

      case VTK_FLOAT:
        fArray = ((vtkFloatArray*)Data)->GetPointer(0);
        this->Controller->
          Send((float*)&fArray[start], size, sendTo, sendTag);
        break;

      case VTK_DOUBLE:
        dArray = ((vtkDoubleArray*)Data)->GetPointer(0);
        dataSize = sizeof(double);
        this->Controller->
          Send((char*)&dArray[start], size*dataSize, sendTo, sendTag);
        break;

      case VTK_LONG:
        lArray = ((vtkLongArray*)Data)->GetPointer(0);
        dataSize = sizeof(long);
        this->Controller->
          Send((char*)&lArray[start], size*dataSize, sendTo, sendTag);
        break;
        
      case VTK_ID_TYPE:
        idArray = ((vtkIdTypeArray*)Data)->GetPointer(0);
        dataSize = sizeof(vtkIdType);
        this->Controller->
          Send((char*)&idArray[start], size*dataSize, sendTo, sendTag);
        break;
        
      case VTK_BIT:
        cerr<<"VTK_BIT not allowed for send"<<endl;
        break;
      case VTK_UNSIGNED_SHORT:
        cerr<<"VTK_UNSIGNED_SHORT not allowed for send"<<endl;
        break;
      case VTK_SHORT:
        cerr<<"VTK_SHORT not allowed for send"<<endl;
        break;
      case VTK_UNSIGNED_INT:
        cerr<<"VTK_UNSIGNED_INT not allowed for send"<<endl;
        break;
      default:
        cerr<<"datatype = "<<dataType<<" not allowed for send"<<endl;
   }
}
//*****************************************************************
// ... Receive the attribute data from recFrom.  Call AllocateDataArrays
//   before calling this ...

void vtkRedistributePoly::ReceiveDataArrays
   (vtkDataSetAttributes* toPd, const vtkIdType numToCopy, const int recFrom,
    vtkIdType* toId, const int typetag)
{

  // ... this assumes that memory has been allocated already, this is
  //     helpful to avoid repeatedly resizing ...

  if ( toPd->GetCopyScalars() )
  {
    vtkDataArray* toScalars = toPd->GetScalars();
    if (toScalars != NULL)
    {
      int activeComponent = 0;
      //int activeComponent = toScalars->GetActiveComponent();
      vtkDataArray* Data = toScalars;
      int recTag = VTK_SCALARS_TAG+typetag;

      ReceiveArrays (Data, numToCopy, recFrom, toId, activeComponent, recTag);
    }
  }

  if ( toPd->GetCopyVectors() )
  {
    vtkDataArray* toVectors = toPd->GetVectors();
    if (toVectors != NULL)
    {
      vtkDataArray* Data = toVectors;
      int recTag = VTK_VECTORS_TAG+typetag;
      int activeComponent = -1;
      ReceiveArrays (Data, numToCopy, recFrom, toId, activeComponent, recTag);
    }
  }

  if ( toPd->GetCopyNormals() )
  {
    vtkDataArray* toNormals = toPd->GetNormals();
    if (toNormals != NULL)
    {
      vtkDataArray* Data = toNormals;
      int recTag = VTK_NORMALS_TAG+typetag;
      int activeComponent = -1;
      ReceiveArrays (Data, numToCopy, recFrom, toId, activeComponent, recTag);
    }
  }

  if ( toPd->GetCopyTCoords() )
  {
    vtkDataArray* toTCoords   = toPd->GetTCoords();
    if (toTCoords != NULL)
    {
      vtkDataArray* Data = toTCoords;
      int recTag = VTK_TCOORDS_TAG+typetag;
      int activeComponent = -1;
      ReceiveArrays (Data, numToCopy, recFrom, toId, activeComponent, recTag);
    }
  }

  if ( toPd->GetCopyTensors() )
  {
    vtkDataArray* toTensors = toPd->GetTensors();
    if (toTensors != NULL)
    {
      vtkDataArray* Data = toTensors;
      int recTag = VTK_TENSOR_TAG+typetag;
      int activeComponent = -1;
      ReceiveArrays (Data, numToCopy, recFrom, toId, activeComponent, recTag);
    }
  }
#if 0
  //if ( toPd->GetCopyFieldData() )
  {
    vtkFieldData* toFieldData = toPd->GetFieldData();
    if (toFieldData != NULL)
    {
      int numArrays=toFieldData->GetNumberOfArrays();
      for (int j=0; j<numArrays; j++)
      {
        vtkDataArray* Data = toFieldData->GetArray(j);
        int recTag = VTK_FIELDDATA_TAG+typetag+j*100;
        int activeComponent = -1;
        ReceiveArrays (Data, numToCopy, recFrom, toId, activeComponent, recTag);
      }
    }
  }
#endif
}
//*****************************************************************************
void vtkRedistributePoly::ReceiveArrays
   (vtkDataArray* Data, const vtkIdType numToCopy, const int recFrom,
    vtkIdType* toId, const int activeComponentInp, const int recTag)
//*****************************************************************************
{
   char* sc;
   char *cArray;
   int *iArray, *si;
   float *fArray, *sf;
   long *lArray, *sl;
   vtkIdType *idArray, *sid;
   unsigned long *ulArray, *sul;
   unsigned char *ucArray, *suc;
   double *dArray, *sd;
   int dataSize;
   int numComps = Data->GetNumberOfComponents();
   int dataType = Data->GetDataType();

   int numCompsToCopy;
   int activeComponent = activeComponentInp;

   vtkIdType i;
   int j;

   if (activeComponent>= 0) 
     // ... scalar case ...
     numCompsToCopy = 1;
   else
   {
     // ... all other cases ...
     numCompsToCopy = numComps;
     activeComponent = 0;
   }

   switch (dataType)
   {
      case VTK_CHAR:
        cArray = ((vtkCharArray*)Data)->GetPointer(0);
        sc = new char[numToCopy*numCompsToCopy];

        this->Controller->
          Receive(sc, numToCopy*numCompsToCopy, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            cArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              sc[numCompsToCopy*i+j];

        delete [] sc;
        break;

      case VTK_UNSIGNED_CHAR:
        ucArray = ((vtkUnsignedCharArray*)Data)->GetPointer(0);
        suc = new unsigned char[numToCopy*numCompsToCopy];

        this->Controller->
          Receive((char*)suc, numToCopy*numCompsToCopy, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            ucArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              suc[numCompsToCopy*i+j];

        delete [] suc;
        break;

      case VTK_INT:
        iArray = ((vtkIntArray*)Data)->GetPointer(0);
        si = new int[numToCopy*numCompsToCopy];

        this->Controller->
          Receive(si, numToCopy*numCompsToCopy, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            iArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              si[numCompsToCopy*i+j];

           delete [] si;
        break;

      case VTK_UNSIGNED_LONG:
        ulArray = 
          ((vtkUnsignedLongArray*)Data)->GetPointer(0);
        sul = new unsigned long [numToCopy*numCompsToCopy];

        this->Controller->
          Receive(sul, numToCopy*numCompsToCopy, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            ulArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              sul[numCompsToCopy*i+j];
        
           delete [] sul;
        break;

      case VTK_FLOAT:
        fArray = ((vtkFloatArray*)Data)->GetPointer(0);
        sf = new float[numToCopy*numCompsToCopy];

        this->Controller->
          Receive(sf, numToCopy*numCompsToCopy, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            fArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              sf[numCompsToCopy*i+j];
        
        delete [] sf;
        break;

      case VTK_DOUBLE:
        dArray = ((vtkDoubleArray*)Data)->GetPointer(0);
        dataSize = sizeof(double);
        sc = (char*)new char[numToCopy*numCompsToCopy*dataSize];
        sd = (double*)sc;

        this->Controller->
          Receive(sc, numToCopy*numCompsToCopy*dataSize, recFrom, recTag);
        if (!colorProc)
        {
           for (i = 0; i < numToCopy; i++)
             for (j = 0; j < numCompsToCopy; j++)
               dArray[toId[i]*numCompsToCopy+j+activeComponent] = 
                 sd[numCompsToCopy*i+j];
        }
        else
        {
           for (i = 0; i < numToCopy; i++)
             for (j = 0; j < numCompsToCopy; j++)
               dArray[toId[i]*numCompsToCopy+j+activeComponent] = recFrom;
        }

        delete [] sc;
        break;

      case VTK_LONG:
        lArray = ((vtkLongArray*)Data)->GetPointer(0);
        dataSize = sizeof(long);
        sc = (char*)new long[numToCopy*numCompsToCopy*dataSize];
        sl = (long*)sc;

        this->Controller->
          Receive(sc, numToCopy*numCompsToCopy*dataSize, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            lArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              sl[numCompsToCopy*i+j];

        delete [] sc;
        break;
        
      case VTK_ID_TYPE:
        idArray = ((vtkIdTypeArray*)Data)->GetPointer(0);
        dataSize = sizeof(vtkIdType);
        sc = (char*)new vtkIdType[numToCopy*numCompsToCopy*dataSize];
        sid = (vtkIdType*)sc;

        this->Controller->
          Receive(sc, numToCopy*numCompsToCopy*dataSize, recFrom, recTag);
        for (i = 0; i < numToCopy; i++)
          for (j = 0; j < numCompsToCopy; j++)
            idArray[toId[i]*numCompsToCopy+j+activeComponent] = 
              sid[numCompsToCopy*i+j];

        delete [] sc;
        break;
        
      case VTK_BIT:
        cerr<<"VTK_BIT not allowed for receive"<<endl;
        break;
      case VTK_UNSIGNED_SHORT:
        cerr<<"VTK_UNSIGNED_SHORT not allowed for receive"<<endl;
        break;
      case VTK_SHORT:
        cerr<<"VTK_SHORT not allowed for receive"<<endl;
        break;
      case VTK_UNSIGNED_INT:
        cerr<<"VTK_UNSIGNED_INT not allowed for receive"<<endl;
        break;
      default:
        cerr<<"datatype = "<<dataType<<" not allowed for receive"<<endl;
   }
}

//--------------------------------------------------------------------
void vtkRedistributePoly::CompleteArrays(const int recFrom)
{
  int j;

  int num;
  vtkDataArray *array;
  char *name;
  int nameLength;
  int type;
  int numComps;
  int index;
  int attributeType;
  int copyFlag;

  vtkPolyData* output = this->GetOutput();


  // First Point data.

  this->Controller->Receive(&num, 1, recFrom, 997244);
  for (j = 0; j < num; ++j)
  {
    this->Controller->Receive(&type, 1, recFrom, 997245);
    switch (type)
      {
      case VTK_INT:
        array = vtkIntArray::New();
        break;
      case VTK_FLOAT:
        array = vtkFloatArray::New();
        break;
      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;
      case VTK_CHAR:
        array = vtkCharArray::New();
        break;
      case VTK_LONG:
        array = vtkLongArray::New();
        break;
      case VTK_SHORT:
        array = vtkShortArray::New();
        break;
      case VTK_UNSIGNED_CHAR:
        array = vtkUnsignedCharArray::New();
        break;
      case VTK_UNSIGNED_INT:
        array = vtkUnsignedIntArray::New();
        break;
      case VTK_UNSIGNED_LONG:
        array = vtkUnsignedLongArray::New();
        break;
      case VTK_UNSIGNED_SHORT:
        array = vtkUnsignedShortArray::New();
        break;
      }
    this->Controller->Receive(&numComps, 1, recFrom, 997246);
    array->SetNumberOfComponents(numComps);
    this->Controller->Receive(&nameLength, 1, recFrom, 997247);
    name = new char[nameLength];
    this->Controller->Receive(name, nameLength, recFrom, 997248);
    array->SetName(name);
    delete [] name;

    index = output->GetPointData()->AddArray(array);

    this->Controller->Receive(&attributeType, 1, recFrom, 997249);
    this->Controller->Receive(&copyFlag, 1, recFrom, 997250);

    if (attributeType != -1 && copyFlag)
      output->GetPointData()->SetActiveAttribute(index, attributeType);

    array->Delete();
  } // end of loop over point arrays.


  // Next Cell data.

  this->Controller->Receive(&num, 1, recFrom, 997244);
  for (j = 0; j < num; ++j)
  {
    this->Controller->Receive(&type, 1, recFrom, 997245);
    switch (type)
      {
      case VTK_INT:
        array = vtkIntArray::New();
        break;
      case VTK_FLOAT:
        array = vtkFloatArray::New();
        break;
      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;
      case VTK_CHAR:
        array = vtkCharArray::New();
        break;
      case VTK_LONG:
        array = vtkLongArray::New();
        break;
      case VTK_SHORT:
        array = vtkShortArray::New();
        break;
      case VTK_UNSIGNED_CHAR:
        array = vtkUnsignedCharArray::New();
        break;
      case VTK_UNSIGNED_INT:
        array = vtkUnsignedIntArray::New();
        break;
      case VTK_UNSIGNED_LONG:
        array = vtkUnsignedLongArray::New();
        break;
      case VTK_UNSIGNED_SHORT:
        array = vtkUnsignedShortArray::New();
        break;
      }
    this->Controller->Receive(&numComps, 1, recFrom, 997246);
    array->SetNumberOfComponents(numComps);
    this->Controller->Receive(&nameLength, 1, recFrom, 997247);
    name = new char[nameLength];
    this->Controller->Receive(name, nameLength, recFrom, 997248);
    array->SetName(name);
    delete [] name;
    index = output->GetCellData()->AddArray(array);

    this->Controller->Receive(&attributeType, 1, recFrom, 997249);
    this->Controller->Receive(&copyFlag, 1, recFrom, 997250);

    if (attributeType != -1 && copyFlag)
      output->GetCellData()->SetActiveAttribute(index, attributeType);

    array->Delete();
  } // end of loop over cell arrays.
  
}



//-----------------------------------------------------------------------
void vtkRedistributePoly::SendCompleteArrays (const int sendTo)
{
  int num;
  int i;
  int type;
  int numComps;
  int nameLength;
  const char *name;
  vtkDataArray *array;
  int attributeType; 
  int copyFlag;

  vtkPolyData* input = this->GetInput();

  // First point data.
  num = input->GetPointData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, sendTo, 997244);
  for (i = 0; i < num; ++i)
  {
    array = input->GetPointData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, sendTo, 997245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, sendTo, 997246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = strlen(name)+1;
    this->Controller->Send(&nameLength, 1, sendTo, 997247);
    // I am pretty sure that Send does not modify the string.
    this->Controller->Send(const_cast<char*>(name), nameLength, 
                           sendTo, 997248);

    attributeType = input->GetPointData()->IsArrayAnAttribute(i);
    copyFlag = -1;
    if (attributeType != -1) 
    {
       // ... Note: this would be much simpler if there was a 
       //    GetCopyAttributeFlag function or if the variable 
       //    wasn't protected. ...
       switch (attributeType)
       {
         case vtkDataSetAttributes::SCALARS:
           copyFlag = input->GetPointData()->GetCopyScalars();
           break;

         case vtkDataSetAttributes::VECTORS: 
           copyFlag = input->GetPointData()->GetCopyVectors(); 
           break;

         case vtkDataSetAttributes::NORMALS:
           copyFlag = input->GetPointData()->GetCopyNormals();
           break;

         case vtkDataSetAttributes::TCOORDS:
           copyFlag = input->GetPointData()->GetCopyTCoords();
           break;

         case vtkDataSetAttributes::TENSORS:
           copyFlag = input->GetPointData()->GetCopyTensors();
           break;

         default:
           copyFlag = 0;

       }
    }
    this->Controller->Send(&attributeType, 1, sendTo, 997249);
    this->Controller->Send(&copyFlag, 1, sendTo, 997250);

  }

  // Next cell data.
  num = input->GetCellData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, sendTo, 997244);
  for (i = 0; i < num; ++i)
  {
    array = input->GetCellData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, sendTo, 997245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, sendTo, 997246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = strlen(name+1);
    this->Controller->Send(&nameLength, 1, sendTo, 997247);
    this->Controller->Send(const_cast<char*>(name), nameLength, sendTo, 
                           997248);
    attributeType = input->GetCellData()->IsArrayAnAttribute(i);
    copyFlag = -1;
    if (attributeType != -1) 
    {
       // ... Note: this would be much simpler if there was a 
       //    GetCopyAttributeFlag function or if the variable 
       //    wasn't protected. ...
       switch (attributeType)
       {
         case vtkDataSetAttributes::SCALARS:
           copyFlag = input->GetCellData()->GetCopyScalars();
           break;

         case vtkDataSetAttributes::VECTORS: 
           copyFlag = input->GetCellData()->GetCopyVectors(); 
           break;

         case vtkDataSetAttributes::NORMALS:
           copyFlag = input->GetCellData()->GetCopyNormals();
           break;

         case vtkDataSetAttributes::TCOORDS:
           copyFlag = input->GetCellData()->GetCopyTCoords();
           break;

         case vtkDataSetAttributes::TENSORS:
           copyFlag = input->GetCellData()->GetCopyTensors();
           break;

         default:
           copyFlag = 0;

       }
    }
    this->Controller->Send(&attributeType, 1, sendTo, 997249);
    this->Controller->Send(&copyFlag, 1, sendTo, 997250);
  }
}


//======================================================================
