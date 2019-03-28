/*=========================================================================

  Program:   ParaView
  Module:    vtkRedistributePolyData.cxx

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
#include "vtkRedistributePolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include "vtkSetGet.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkRedistributePolyData);

vtkCxxSetObjectMacro(vtkRedistributePolyData, Controller, vtkMultiProcessController);

#define VTK_REDIST_DO_TIMING 0
#define NUM_CELL_TYPES 4

typedef struct
{
  vtkTimerLog* timer;
  float time;
} _TimerInfo;
_TimerInfo timerInfo8;

vtkRedistributePolyData::vtkRedistributePolyData()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->ColorProc = 0;
}

vtkRedistributePolyData::~vtkRedistributePolyData()
{
  this->SetController(0);
}

int vtkRedistributePolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
#if VTK_REDIST_DO_TIMING
  vtkTimerLog* timer8 = vtkTimerLog::New();
  timerInfo8.timer = timer8;

  // timerInfo8.time = 0.;
  timerInfo8.timer->StartTimer();
#endif

  vtkPolyData* tmp = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  vtkPolyData* input = vtkPolyData::New();
  input->ShallowCopy(tmp);
  this->CompleteInputArrays(input);

  // Check for bad input that would make us hang.
  if (!this->DoubleCheckArrays(input))
  {
    vtkDebugMacro("Skipping redistribute to avoid hanging.");
    output->CopyStructure(tmp);
    output->GetPointData()->ShallowCopy(tmp->GetPointData());
    output->GetCellData()->ShallowCopy(tmp->GetCellData());
    input->Delete();
    return 1;
  }

  int myId;

  if (!this->Controller)
  {
    vtkErrorMacro("need controller to redistribute cells");
    input->Delete();
    return 0;
  }
  myId = this->Controller->GetLocalProcessId();

  // ... make schedule of how many and where to ship polys ...

  this->Controller->Barrier();

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro("barrier bef sched time = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif

  vtkCommSched localSched;
  this->MakeSchedule(input, &localSched);
  this->OrderSchedule(&localSched); // order schedule to avoid
  // blocking problems later
  vtkIdType*** sendCellList = localSched.SendCellList;
  vtkIdType** keepCellList = localSched.KeepCellList;
  int* sendTo = localSched.SendTo;
  int* recFrom = localSched.ReceiveFrom;
  int cntSend = localSched.SendCount;
  int cntRec = localSched.ReceiveCount;
  vtkIdType** sendNum = localSched.SendNumber;
  vtkIdType** recNum = localSched.ReceiveNumber;
  vtkIdType* numCells = localSched.NumberOfCells;

#if VTK_REDIST_DO_TIMING
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro(<< "schedule time = " << timerInfo8.time);
  }
  timerInfo8.timer->StartTimer();
#endif

// beginning of turned of bounds section (not needed)
#if 0
  // ... expand bounds on all processors to be the maximum on any processor ...

  this->Controller->Barrier();

#if VTK_REDIST_DO_TIMING
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId==0)
    {
    vtkDegugMacro(<<"barrier bef bounds time = "<<timerInfo8.time);
    }
  timerInfo8.timer->StartTimer();
#endif


  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();

  float *bounds = input->GetBounds(), *remoteBounds = bounds;

  for (id = 0; id < numProcs; id++)
    {
    // ... send out bounds to all the other processors ...
    if (id != myId)
      {
      this->Controller->Send(input->GetBounds(), 6, id, BOUNDS_TAG);
      }
    }
  
  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
      // ... get remote bounds and expand bounds to include these ...
      this->Controller->Receive(remoteBounds, 6, id, BOUNDS_TAG);
      if (remoteBounds[0] < bounds[0]) bounds[0] = remoteBounds[0];
      if (remoteBounds[1] > bounds[1]) bounds[1] = remoteBounds[1];
      if (remoteBounds[2] < bounds[2]) bounds[2] = remoteBounds[2];
      if (remoteBounds[3] > bounds[3]) bounds[3] = remoteBounds[3];
      if (remoteBounds[4] < bounds[4]) bounds[4] = remoteBounds[4];
      if (remoteBounds[5] > bounds[5]) bounds[5] = remoteBounds[5];
      }
    }

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId==0)
    {
    vtkDebugMacro(<<" bounds time = "<<timerInfo8.Time);
    }
  timerInfo8.Timer->StartTimer();
#endif

#endif // end of turned of bounds section

  // ... allocate space and copy point and cell data attributes from input to
  //   output ...

  output->GetPointData()->CopyGlobalIdsOn();
  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetCellData()->CopyGlobalIdsOn();
  output->GetCellData()->CopyAllocate(input->GetCellData());

  // ... copy remaining input cell data to output cell data ...

  vtkCellArray* inputCellArrays[NUM_CELL_TYPES];
  inputCellArrays[0] = input->GetVerts();
  inputCellArrays[1] = input->GetLines();
  inputCellArrays[2] = input->GetPolys();
  inputCellArrays[3] = input->GetStrips();

  int i, type;
  vtkIdType inputNumCells[NUM_CELL_TYPES];
  vtkIdType origNumCells[NUM_CELL_TYPES];
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (inputCellArrays[type])
    {
      inputNumCells[type] = inputCellArrays[type]->GetNumberOfCells();
    }
    else
    {
      inputNumCells[type] = 0;
    }
    origNumCells[type] = inputNumCells[type];

    // check to see if number of cells is less than original and
    // only copy that many or copy all of input cells if extra are
    // added

    if (numCells[type] < origNumCells[type])
    {
      origNumCells[type] = numCells[type];
    }
  }

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro(<< "copy alloctime = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif

  // sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
  // ... send cell and point sizes ...

  vtkIdType prevStopCell[NUM_CELL_TYPES];
  vtkIdType startCell[NUM_CELL_TYPES];
  vtkIdType stopCell[NUM_CELL_TYPES];
  vtkIdType totalNumCellsToSend[NUM_CELL_TYPES];

  // ... In certain cases cells may not only be copied on processor
  //   but the same cells may also be sent to another processor.
  //   Move the start sending cell back so that there are enough
  //   cells to send. ..

  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    totalNumCellsToSend[type] = 0;
    for (i = 0; i < cntSend; i++)
    {
      totalNumCellsToSend[type] += sendNum[type][i];
    }
    prevStopCell[type] = origNumCells[type] - 1;
    if (totalNumCellsToSend[type] + origNumCells[type] > inputNumCells[type])
    {
      prevStopCell[type] = inputNumCells[type] - totalNumCellsToSend[type] - 1;
    }
  }
  vtkIdType* numPointsSend = new vtkIdType[cntSend];
  vtkIdType** cellArraySize = new vtkIdType*[cntSend];

  for (i = 0; i < cntSend; i++)
  {
    cellArraySize[i] = new vtkIdType[NUM_CELL_TYPES];

    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      startCell[type] = prevStopCell[type] + 1;
      stopCell[type] = startCell[type] + sendNum[type][i] - 1;
      prevStopCell[type] = stopCell[type];
    }
    if (sendCellList)
    {
      this->SendCellSizes(
        startCell, stopCell, input, sendTo[i], numPointsSend[i], cellArraySize[i], sendCellList[i]);
    }
    else
    {
      this->SendCellSizes(
        startCell, stopCell, input, sendTo[i], numPointsSend[i], cellArraySize[i], NULL);
    }

  } // end of list of processors to send to

// ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

// ... allocate memory before receiving data ...

#if (VTK_REDIST_DO_TIMING == 1)
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro(<< "send sizes time = " << timerInfo8.time);
  }
  timerInfo8.timer->StartTimer();
#endif

  // ... find memory requirements for on processor copy ...
  vtkIdType numPointsOnProc = 0;
  vtkIdType numCellPtsOnProc[NUM_CELL_TYPES];
  this->FindMemReq(origNumCells, input, numPointsOnProc, numCellPtsOnProc);

#if VTK_REDIST_DO_TIMING
  timerInfo8.timer->StopTimer();
  timerInfo8.time += timerInfo8.timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro(<< "mem req time = " << timerInfo8.time);
  }
  timerInfo8.timer->StartTimer();
#endif
  vtkIdType* numPointsRec = new vtkIdType[cntRec];
  vtkIdType** cellptCntr = new vtkIdType*[cntRec];
  for (i = 0; i < cntRec; i++)
  {
    cellptCntr[i] = new vtkIdType[NUM_CELL_TYPES];
  }

  for (i = 0; i < cntRec; i++)
  {
    this->Controller->Receive((vtkIdType*)cellptCntr[i], NUM_CELL_TYPES, recFrom[i], CELL_CNT_TAG);
    this->Controller->Receive((vtkIdType*)&numPointsRec[i], 1, recFrom[i], POINTS_SIZE_TAG);
  }
  vtkCellData* outputCellData = output->GetCellData();
  vtkPointData* outputPointData = output->GetPointData();

  this->AllocateCellDataArrays(outputCellData, recNum, cntRec, origNumCells);
  this->AllocatePointDataArrays(outputPointData, numPointsRec, cntRec, numPointsOnProc);

  vtkIdType totalNumPoints = numPointsOnProc;
  vtkIdType totalNumCells[NUM_CELL_TYPES];
  vtkIdType totalNumCellPts[NUM_CELL_TYPES];
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    totalNumCells[type] = origNumCells[type];
    totalNumCellPts[type] = numCellPtsOnProc[type];
  }

  for (i = 0; i < cntRec; i++)
  {
    totalNumPoints += numPointsRec[i];
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      totalNumCells[type] += recNum[type][i];
      totalNumCellPts[type] += cellptCntr[i][type];
    }
  }

  vtkSmartPointer<vtkPoints> outputPoints = vtkSmartPointer<vtkPoints>::New();
  outputPoints->SetNumberOfPoints(totalNumPoints);

  vtkSmartPointer<vtkCellArray> outputVerts;
  vtkSmartPointer<vtkCellArray> outputLines;
  vtkSmartPointer<vtkCellArray> outputPolys;
  vtkSmartPointer<vtkCellArray> outputStrips;

  if (inputCellArrays[0])
  {
    outputVerts = vtkSmartPointer<vtkCellArray>::New();
  }
  if (inputCellArrays[1])
  {
    outputLines = vtkSmartPointer<vtkCellArray>::New();
  }
  if (inputCellArrays[2])
  {
    outputPolys = vtkSmartPointer<vtkCellArray>::New();
  }
  if (inputCellArrays[3])
  {
    outputStrips = vtkSmartPointer<vtkCellArray>::New();
  }

  vtkCellArray* outputCellArrays[NUM_CELL_TYPES];
  outputCellArrays[0] = outputVerts.GetPointer();
  outputCellArrays[1] = outputLines.GetPointer();
  outputCellArrays[2] = outputPolys.GetPointer();
  outputCellArrays[3] = outputStrips.GetPointer();

  vtkIdType* ptr = 0;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (totalNumCellPts[type] > 0)
    {
      if (outputCellArrays[type])
      {
        ptr = outputCellArrays[type]->WritePointer(totalNumCells[type], totalNumCellPts[type]);
        if (ptr == 0)
        {
          vtkErrorMacro("Error: can't allocate points.");
        }
      }
    }
  }
  output->SetVerts(outputVerts.GetPointer());
  output->SetLines(outputLines.GetPointer());
  output->SetPolys(outputPolys.GetPointer());
  output->SetStrips(outputStrips.GetPointer());

  output->SetPoints(outputPoints.GetPointer());
  // aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
  // ... Copy cells from input to output ...
  this->CopyCells(origNumCells, input, output, keepCellList);

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro("copy cells time = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif

  // eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee

  // ... first exchange cells between processors.  Do this by
  //  receiving first if this processor number is less than the
  //  one it is exchanging with else send first ...

  vtkIdType prevStopCellRec[NUM_CELL_TYPES];
  vtkIdType prevStopCellSend[NUM_CELL_TYPES];

  vtkIdType prevNumPointsRec = numPointsOnProc;
  vtkIdType prevCellptCntrRec[NUM_CELL_TYPES];

  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    prevStopCellRec[type] = origNumCells[type] - 1;
    prevStopCellSend[type] = origNumCells[type] - 1;
    prevCellptCntrRec[type] = numCellPtsOnProc[type];

    if (totalNumCellsToSend[type] + origNumCells[type] > inputNumCells[type])
    {
      prevStopCellSend[type] = inputNumCells[type] - totalNumCellsToSend[type] - 1;
    }
  }

  int finished = 0;
  int procRec, procSend;
  int rcntr = 0;
  int scntr = 0;
  int receiving;

  while (!finished && (cntRec > 0 || cntSend > 0))
  {
    if (rcntr < cntRec)
    {
      procRec = recFrom[rcntr];
    }
    else
    {
      procRec = 99999;
    }
    if (scntr < cntSend)
    {
      procSend = sendTo[scntr];
    }
    else
    {
      procSend = 99999;
    }

    receiving = 0;

    // ... send or receive the smallest processor number next
    //   (will send if receive == 0) ...
    if (procRec < procSend)
    {
      receiving = 1;
    }
    else if (procRec == procSend)
    {
      // ... an exchange between 2 prcessors ...
      if (myId < procRec)
      {
        receiving = 1;
      }
    }

    if (receiving)
    {
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        startCell[type] = prevStopCellRec[type] + 1;
        stopCell[type] = startCell[type] + recNum[type][rcntr] - 1;
      }

      this->ReceiveCells(startCell, stopCell, output, recFrom[rcntr], prevCellptCntrRec,
        cellptCntr[rcntr], prevNumPointsRec, numPointsRec[rcntr]);

      prevNumPointsRec += numPointsRec[rcntr];
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        prevCellptCntrRec[type] += cellptCntr[rcntr][type];
        prevStopCellRec[type] = stopCell[type];
      }
      rcntr++;
    }
    else
    {
      // ... sending ...
      if (sendCellList == NULL)
      {
        for (type = 0; type < NUM_CELL_TYPES; type++)
        {
          startCell[type] = prevStopCellSend[type] + 1;
          stopCell[type] = startCell[type] + sendNum[type][scntr] - 1;
        }
        this->SendCells(startCell, stopCell, input, output, sendTo[scntr], numPointsSend[scntr],
          cellArraySize[scntr], NULL);
      }
      else
      {
        for (type = 0; type < NUM_CELL_TYPES; type++)
        {
          startCell[type] = 0;
          stopCell[type] = sendNum[type][scntr] - 1;
        }
        this->SendCells(startCell, stopCell, input, output, sendTo[scntr], numPointsSend[scntr],
          cellArraySize[scntr], sendCellList[scntr]);
      }

      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        prevStopCellSend[type] = stopCell[type];
      }
      scntr++;
    }

    if (scntr >= cntSend && rcntr >= cntRec)
    {
      finished = 1;
    }
  }

  input->Delete();
  input = NULL;

// eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == 0)
  {
    vtkDebugMacro("send/rec (at end) time = " << timerInfo8.Time);
  }
#endif

  delete[] numPointsRec;
  delete[] cellptCntr;
  delete[] cellArraySize;
  return 1;
}

//*****************************************************************
void vtkRedistributePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller :";
  if (this->Controller)
  {
    os << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "ColorProc :" << this->ColorProc << "\n";
}

//*****************************************************************
void vtkRedistributePolyData::MakeSchedule(vtkPolyData* input, vtkCommSched* localSched)

{
  //*****************************************************************
  // purpose: This routine sets up a schedule that does nothing.
  //
  //*****************************************************************

  // get total number of polys and figure out how many each
  // processor should have

  vtkCellArray* cellArrays[NUM_CELL_TYPES];
  cellArrays[0] = input->GetVerts();
  cellArrays[1] = input->GetLines();
  cellArrays[2] = input->GetPolys();
  cellArrays[3] = input->GetStrips();

  // ... initialize the local schedule to return ...
  int type;
  localSched->NumberOfCells = new vtkIdType[NUM_CELL_TYPES];
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (cellArrays[type])
    {
      localSched->NumberOfCells[type] = cellArrays[type]->GetNumberOfCells();
    }
    else
    {
      localSched->NumberOfCells[type] = 0;
    }
  }

  localSched->SendCount = 0;
  localSched->ReceiveCount = 0;
  localSched->SendTo = NULL;
  localSched->SendNumber = NULL;
  localSched->ReceiveFrom = NULL;
  localSched->ReceiveNumber = NULL;
  localSched->SendCellList = NULL;
  localSched->KeepCellList = NULL;
}
//*****************************************************************
void vtkRedistributePolyData::OrderSchedule(vtkCommSched* localSched)

{
  vtkIdType*** sendCellList = localSched->SendCellList;
  vtkIdType** sendNum = localSched->SendNumber;
  vtkIdType** recNum = localSched->ReceiveNumber;
  int* sendTo = localSched->SendTo;
  int* recFrom = localSched->ReceiveFrom;
  int cntSend = localSched->SendCount;
  int cntRec = localSched->ReceiveCount;

  // ... first find number of exchanges where a processor both
  //  sends and receives from the same processor ...

  int i, j;
  int* order;
  int temp;
  int tempid;
  vtkIdType* templist;
  int temporder;
  int type;

  // ... first order sends and then receives to avoid blocking
  //  problems later ...

  int outoforder; // flag to determine if schedule is out of order
  if (cntSend > 0)
  {
    outoforder = 0;
    order = new int[cntSend];
    for (i = 0; i < cntSend; i++)
    {
      order[i] = i;
    }
    for (i = 0; i < cntSend; i++)
    {
      for (j = i + 1; j < cntSend; j++)
      {
        if (sendTo[i] > sendTo[j])
        {
          temp = order[i];
          order[i] = order[j];
          order[j] = temp;
          outoforder = 1;
        }
      }
    }

    // ... now reorder the sends ...
    if (outoforder)
    {
      for (i = 0; i < cntSend; i++)
      {
        while (order[i] != i)
        {
          temporder = order[i];

          temp = sendTo[i];
          sendTo[i] = sendTo[temporder];
          sendTo[temporder] = temp;

          for (type = 0; type < NUM_CELL_TYPES; type++)
          {
            tempid = sendNum[type][i];
            sendNum[type][i] = sendNum[type][temporder];
            sendNum[type][temporder] = tempid;
          }

          if (sendCellList != NULL)
          {
            for (type = 0; type < NUM_CELL_TYPES; type++)
            {
              templist = sendCellList[i][type];
              sendCellList[i][type] = sendCellList[temporder][type];
              sendCellList[temporder][type] = templist;
            }
          }

          temporder = order[i];
          order[i] = order[temporder];
          order[temporder] = temporder;
        } // end of while
      }   // end of loop over cntSend
    }     // end if outoforder
    delete[] order;
  } // end of cntSend>0

  if (cntRec > 0)
  {
    outoforder = 0;
    order = new int[cntRec];
    for (i = 0; i < cntRec; i++)
    {
      order[i] = i;
    }
    for (i = 0; i < cntRec; i++)
    {
      for (j = i + 1; j < cntRec; j++)
      {
        if (recFrom[i] > recFrom[j])
        {
          temp = order[i];
          order[i] = order[j];
          order[j] = temp;
          outoforder = 1;
        }
      }
    }
    // ... now reorder the receives ...
    if (outoforder)
    {
      for (i = 0; i < cntRec; i++)
      {
        while (order[i] != i)
        {
          temporder = order[i];

          temp = recFrom[i];
          recFrom[i] = recFrom[temporder];
          recFrom[temporder] = temp;

          for (type = 0; type < NUM_CELL_TYPES; type++)
          {
            tempid = recNum[type][i];
            recNum[type][i] = recNum[type][temporder];
            recNum[type][temporder] = tempid;
          }

          temporder = order[i];
          order[i] = order[temporder];
          order[temporder] = temporder;

        } // end while
      }   // end loop over cntRec
    }     // end if outoforder
    delete[] order;
  } // end if cnrRec>0
}
//*****************************************************************
//*****************************************************************
// Copy the attribute data from one id to another. Make sure CopyAllocate() has// been invoked
// before using this method.
void vtkRedistributePolyData::CopyDataArrays(vtkDataSetAttributes* fromPd,
  vtkDataSetAttributes* toPd, vtkIdType numToCopy, vtkIdType* fromId, int myId)
{

  vtkDataArray* DataFrom;
  vtkDataArray* DataTo;

  int numArrays = fromPd->GetNumberOfArrays();

  for (int i = 0; i < numArrays; i++)
  {
    DataFrom = fromPd->GetArray(i);
    DataTo = toPd->GetArray(i);

    this->CopyArrays(DataFrom, DataTo, numToCopy, fromId, myId);
  }
}
//*****************************************************************
// Copy the attribute data from one id to another. Make sure
//   CopyAllocate() has been invoked before using this method.

void vtkRedistributePolyData::CopyCellBlockDataArrays(vtkDataSetAttributes* fromPd,
  vtkDataSetAttributes* toPd, vtkIdType numToCopy, vtkIdType startCell, vtkIdType fromOffset,
  vtkIdType toOffset, int myId)
//*******************************************************************
{

  vtkDataArray* DataFrom;
  vtkDataArray* DataTo;

  int numArrays = fromPd->GetNumberOfArrays();

  for (int i = 0; i < numArrays; i++)
  {
    DataFrom = fromPd->GetArray(i);
    DataTo = toPd->GetArray(i);

    this->CopyBlockArrays(DataFrom, DataTo, numToCopy, startCell, fromOffset, toOffset, myId);
  }
}

//------------------------------------------------------------------
namespace
{
template <typename T>
void CopyArraysTemplate(vtkDataArray* DataFrom, vtkDataArray* DataTo, vtkIdType numToCopy,
  vtkIdType* fromId, int myId, bool fillWithMyId)
{
  T* from = (T*)DataFrom->GetVoidPointer(0);
  T* to = (T*)DataTo->GetVoidPointer(0);
  int numComps = DataFrom->GetNumberOfComponents();
  if (fillWithMyId)
  {
    for (vtkIdType i = 0; i < numToCopy; ++i)
    {
      for (int j = 0; j < numComps; ++j)
      {
        to[numComps * i + j] = myId;
      }
    }
  }
  else
  {
    for (vtkIdType i = 0; i < numToCopy; ++i)
    {
      for (int j = 0; j < numComps; ++j)
      {
        to[numComps * i + j] = from[numComps * fromId[i] + j];
      }
    }
  }
}
}
//******************************************************************
void vtkRedistributePolyData::CopyArrays(
  vtkDataArray* DataFrom, vtkDataArray* DataTo, vtkIdType numToCopy, vtkIdType* fromId, int myId)
//******************************************************************
{
  int dataType = DataFrom->GetDataType();

  bool fillWithMyId = (dataType == VTK_DOUBLE) && this->ColorProc;

  switch (dataType)
  {

    // Only change is that vtk unsigned short is now allowed...
    vtkTemplateMacro(
      CopyArraysTemplate<VTK_TT>(DataFrom, DataTo, numToCopy, fromId, myId, fillWithMyId));
    case VTK_BIT:
      vtkErrorMacro("VTK_BIT not allowed for copy");
      break;
    default:
      vtkErrorMacro("datatype = " << dataType << " not allowed for copy");
  }
}
//------------------------------------------------------------------
namespace
{
template <typename T>
void CopyBlockArraysTemplate(vtkDataArray* DataFrom, vtkDataArray* DataTo, vtkIdType numToCopy,
  vtkIdType startCell, vtkIdType fromOffset, vtkIdType toOffset, int myId, bool fillToWithMyId)
{
  T* from = (T*)DataFrom->GetVoidPointer(fromOffset);
  T* to = (T*)DataTo->GetVoidPointer(toOffset);

  int numComps = DataFrom->GetNumberOfComponents();

  vtkIdType start = numComps * startCell;
  vtkIdType size = numToCopy * numComps;
  vtkIdType stop = start + size;

  if (fillToWithMyId)
  {
    for (vtkIdType i = start; i < stop; i++)
    {
      to[i] = myId;
    }
  }
  else
  {
    for (vtkIdType i = start; i < stop; i++)
    {
      to[i] = from[i];
    }
  }
}
}
//******************************************************************
void vtkRedistributePolyData::CopyBlockArrays(vtkDataArray* DataFrom, vtkDataArray* DataTo,
  vtkIdType numToCopy, vtkIdType startCell, vtkIdType fromOffset, vtkIdType toOffset, int myId)
//******************************************************************
{
  int dataType = DataFrom->GetDataType();

  bool fillWithMyId = (dataType == VTK_DOUBLE) && this->ColorProc;

  switch (dataType)
  {
    // Only change is that vtk unsigned short is now allowed...
    vtkTemplateMacro(CopyBlockArraysTemplate<VTK_TT>(
      DataFrom, DataTo, numToCopy, startCell, fromOffset, toOffset, myId, fillWithMyId));
    case VTK_BIT:
      vtkErrorMacro("VTK_BIT not allowed for copy");
      break;
    default:
      vtkErrorMacro("datatype = " << dataType << " not allowed for copy");
  }
}
//*****************************************************************
//*****************************************************************
void vtkRedistributePolyData::CopyCells(
  vtkIdType* numCells, vtkPolyData* input, vtkPolyData* output, vtkIdType** keepCellList)

//*****************************************************************
{
  // ... Copy initial subset of cells and points from input to
  //   output.  This assumes that the cells will be copied from
  //   the beginning of the list. ...

  int myId = this->Controller->GetLocalProcessId();
  vtkIdType cellId, i;

  // ... Copy cell data attribute data (Scalars, Vectors, etc.)...
  vtkCellArray* cellArrays[NUM_CELL_TYPES];
  cellArrays[0] = input->GetVerts();
  cellArrays[1] = input->GetLines();
  cellArrays[2] = input->GetPolys();
  cellArrays[3] = input->GetStrips();

  // ... assume that if there are any arrays in the inputCelldata
  //  it is ordered verts, lines, polygons and strips so that
  //  the first cell in lines corresponds with cell number
  //  equal to the number of vert cells. ...

  vtkIdType cellOffset = 0;
  vtkIdType cellOffsetOut = 0;

  vtkCellData* inputCellData = input->GetCellData();
  vtkCellData* outputCellData = output->GetCellData();

  // ...Since fromId's is used to point to cell data where
  //  data from all of the different types of cells is
  //  combined, use an offset because cellId points
  // to cell locations for the individual type. ...

  int type;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    vtkIdType* fromIds = new vtkIdType[numCells[type]];
    if (keepCellList != NULL)
    {
      for (cellId = 0; cellId < numCells[type]; cellId++)
      {
        fromIds[cellId] = keepCellList[type][cellId] + cellOffset;
      }
    }

    if (keepCellList == NULL)
    {
      vtkIdType startCell = 0;
      this->CopyCellBlockDataArrays(
        inputCellData, outputCellData, numCells[type], startCell, cellOffset, cellOffsetOut, myId);
    }
    else
    {
      this->CopyDataArrays(inputCellData, outputCellData, numCells[type], fromIds, myId);
    }
    if (cellArrays[type])
    {
      cellOffset += cellArrays[type]->GetNumberOfCells();
      cellOffsetOut += numCells[type];
    }
    delete[] fromIds;
  }

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == -7)
  {
    vtkDebugMacro("1st copy data arrays time = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif

  // ... Now copy points and point data. ...

  vtkPoints* outputPoints = output->GetPoints();
  vtkFloatArray* outputPointsArray = vtkFloatArray::SafeDownCast(outputPoints->GetData());
  float* outputPointsArrayData = outputPointsArray->GetPointer(0);

  vtkPoints* inputPoints = input->GetPoints();
  void* inputPointsArrayData = NULL;
  int pointsType = VTK_VOID;
  if (inputPoints != NULL)
  {
    pointsType = inputPoints->GetData()->GetDataType();
    inputPointsArrayData = inputPoints->GetVoidPointer(0);
  }

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == -7)
  {
    vtDebugMacro("alloc  pts time = " << timerInfo8.Time)
  }
  timerInfo8.Timer->StartTimer();
#endif

  // ... Allocate maximum possible number of points (use total
  //  from all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* fromPtIds = new vtkIdType[numPointsMax];

  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i = 0; i < numPointsMax; i++)
  {
    usedIds[i] = -1;
  }

  // ... Copy point Id's for all the points in the cell. ...

  vtkCellArray* inputCellArrays[NUM_CELL_TYPES];
  inputCellArrays[0] = input->GetVerts();
  inputCellArrays[1] = input->GetLines();
  inputCellArrays[2] = input->GetPolys();
  inputCellArrays[3] = input->GetStrips();

  vtkCellArray* outputCellArrays[NUM_CELL_TYPES];
  outputCellArrays[0] = output->GetVerts();
  outputCellArrays[1] = output->GetLines();
  outputCellArrays[2] = output->GetPolys();
  outputCellArrays[3] = output->GetStrips();

  vtkIdType pointIncr = 0;
  vtkIdType pointId;
  vtkIdType npts;

  vtkIdType* inPtr;
  vtkIdType* ptr;

  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    inPtr = inputCellArrays[type]->GetPointer();
    ptr = outputCellArrays[type]->GetPointer();

    // ... set output number of points to input number of points ...
    if (keepCellList == NULL)
    {
      for (cellId = 0; cellId < numCells[type]; cellId++)
      {
        // ... set output number of points to input number
        //   of points ...
        npts = *inPtr++;
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
        } // end loop over npts
      }   // end loop over numCells
    }     // end if section where keepCellList is null
    else
    {
      vtkIdType prevCellId = 0;
      for (vtkIdType id = 0; id < numCells[type]; id++)
      {
        cellId = keepCellList[type][id];
        for (i = prevCellId; i < cellId; i++)
        {
          npts = *inPtr++;
          inPtr += npts;
        }
        prevCellId = cellId + 1;

        npts = *inPtr++;
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
        } // end loop over npts
      }   // end loop over cells
    }     // end else statement for keepCellList
  }       // end loop over type

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == -7)
  {
    vtkDebugMacro("copy pt ids time = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif

  // ... Copy cell points. ...
  vtkIdType inLoc, outLoc;
  vtkIdType numPoints = pointIncr;
  int j;

  // ... copy x,y,z coordinates ...
  switch (pointsType)
  {
    vtkTemplateMacro(for (i = 0; i < numPoints; i++) {
      inLoc = fromPtIds[i] * 3;
      outLoc = i * 3;
      for (j = 0; j < 3; j++)
      {
        outputPointsArrayData[outLoc + j] =
          static_cast<float>(reinterpret_cast<VTK_TT*>(inputPointsArrayData)[inLoc + j]);
      }
    });
  }

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == -7)
  {
    vtkDebugMacro("copy pts time = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif

  vtkPointData* inputPointData = input->GetPointData();
  vtkPointData* outputPointData = output->GetPointData();

  // ... copy point data arrays ...
  this->CopyDataArrays(inputPointData, outputPointData, numPoints, fromPtIds, myId);
  delete[] fromPtIds;

#if VTK_REDIST_DO_TIMING
  timerInfo8.Timer->StopTimer();
  timerInfo8.Time += timerInfo8.Timer->GetElapsedTime();
  if (myId == -7)
  {
    vtkDebugMacro("setting time = " << timerInfo8.Time);
  }
  timerInfo8.Timer->StartTimer();
#endif
}

//*****************************************************************
void vtkRedistributePolyData::SendCellSizes(vtkIdType* startCell, vtkIdType* stopCell,
  vtkPolyData* input, int sendTo, vtkIdType& numPoints, vtkIdType* ptcntr, vtkIdType** sendCellList)

//*****************************************************************
{
  // ... send cells and point sizes without sending data ...

  vtkIdType cellId, i;
  vtkIdType numCells;

  // int myId = this->Controller->GetLocalProcessId();

  // ... Allocate maximum possible number of points (use total from
  //     all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i = 0; i < numPointsMax; i++)
  {
    usedIds[i] = -1;
  }

  // ... send point Id's for all the points in the cell. ...

  vtkIdType pointIncr = 0;
  vtkIdType pointId;
  vtkIdType npts;
  vtkIdType* inPtr;

  vtkCellArray* cellArrays[NUM_CELL_TYPES];
  cellArrays[0] = input->GetVerts();
  cellArrays[1] = input->GetLines();
  cellArrays[2] = input->GetPolys();
  cellArrays[3] = input->GetStrips();

  int type;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (cellArrays[type])
    {
      inPtr = cellArrays[type]->GetPointer();
      ptcntr[type] = 0; // counts the number of points stored in the
      // cell array plus includes the extra space
      // for each cell that contains the number of
      // points in that cell.

      if (sendCellList == NULL)
      {
        // ... send cells in a block ...
        for (cellId = 0; cellId < startCell[type]; cellId++)
        {
          // ... increment pointers to get to correct starting
          //  point ...
          npts = *inPtr++;
          inPtr += npts;
        }

        for (cellId = startCell[type]; cellId <= stopCell[type]; cellId++)
        {
          // ... set output number of points to input number of
          //   points ...
          npts = *inPtr++;
          ptcntr[type]++;
          for (i = 0; i < npts; i++)
          {
            pointId = *inPtr++;
            if (usedIds[pointId] == -1)
            {
              usedIds[pointId] = pointIncr++;
            }
            ptcntr[type]++;
          }
        }
      }
      else
      {
        // ... there is a specific list of cells to send ...

        vtkIdType prevCellId = 0;
        numCells = stopCell[type] - startCell[type] + 1;

        for (vtkIdType id = 0; id < numCells; id++)
        {

          cellId = sendCellList[type][id];
          for (i = prevCellId; i < cellId; i++)
          {
            // ... increment pointers to get to correct starting
            // point ...
            npts = *inPtr++;
            inPtr += npts;
          }
          prevCellId = cellId + 1;

          // ... set output number of points to input number of
          //   points ...

          npts = *inPtr++;
          ptcntr[type]++;

          for (i = 0; i < npts; i++)
          {
            pointId = *inPtr++;
            if (usedIds[pointId] == -1)
            {
              usedIds[pointId] = pointIncr++;
            }
            ptcntr[type]++;
          }
        } // end loop over cells
      }   // end if sendCellList
    }     // end if cellArrays
  }       // end loop over type

  // ... send sizes first (must be in this order to allocate for
  //   receive)...

  this->Controller->Send((vtkIdType*)ptcntr, NUM_CELL_TYPES, sendTo, CELL_CNT_TAG);

  numPoints = pointIncr;
  this->Controller->Send((vtkIdType*)&numPoints, 1, sendTo, POINTS_SIZE_TAG);
}
//*****************************************************************
void vtkRedistributePolyData::SendCells(vtkIdType* startCell, vtkIdType* stopCell,
  vtkPolyData* input, vtkPolyData* output, int sendTo, vtkIdType& numPoints,
  vtkIdType* cellArraySize, vtkIdType** sendCellList)

//*****************************************************************
{
  // ... send cells, points and associated data from cells in
  //     specified region ...

  vtkIdType cellId, i;

  // int myId = this->Controller->GetLocalProcessId();

  // ... Allocate maximum possible number of points (use total from
  //     all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* fromPtIds = new vtkIdType[numPointsMax];

  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i = 0; i < numPointsMax; i++)
  {
    usedIds[i] = -1;
  }

  // ... send point Id's for all the points in the cell. ...

  vtkCellArray* inputCellArrays[NUM_CELL_TYPES];
  inputCellArrays[0] = input->GetVerts();
  inputCellArrays[1] = input->GetLines();
  inputCellArrays[2] = input->GetPolys();
  inputCellArrays[3] = input->GetStrips();

  vtkIdType* ptr;
  vtkIdType ptcntr[NUM_CELL_TYPES];
  vtkIdType* ptrsav[NUM_CELL_TYPES];

  vtkIdType pointIncr = 0;
  vtkIdType pointId;
  vtkIdType* inPtr;
  vtkIdType npts;

  int type;

  vtkIdType numCells[NUM_CELL_TYPES];
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    inPtr = inputCellArrays[type]->GetPointer();
    ptr = new vtkIdType[cellArraySize[type]];
    ptrsav[type] = ptr;
    ptcntr[type] = 0;
    numCells[type] = stopCell[type] - startCell[type] + 1;

    // ... set output number of points to input number of points ...
    if (sendCellList == NULL)
    {
      // ... send cells in a block ...
      for (cellId = 0; cellId < startCell[type]; cellId++)
      {
        // ... increment pointers to get to correct starting point ...
        npts = *inPtr++;
        inPtr += npts;
      }

      for (cellId = startCell[type]; cellId <= stopCell[type]; cellId++)
      {
        npts = *inPtr++;
        *ptr++ = npts;
        ptcntr[type]++;
        for (i = 0; i < npts; i++)
        {
          pointId = *inPtr++;
          if (usedIds[pointId] == -1)
          {
            vtkIdType newPt = pointIncr;
            *ptr++ = newPt;
            ptcntr[type]++;
            usedIds[pointId] = newPt;
            fromPtIds[pointIncr] = pointId;
            pointIncr++;
          }
          else
          {
            // ... use new point id ...
            *ptr++ = usedIds[pointId];
            ptcntr[type]++;
          }
        } // end loop over npts
      }   // end loop over cellId
    }
    else
    {
      // ... there is a specific list of cells to send ...

      vtkIdType prevCellId = 0;

      for (vtkIdType id = 0; id < numCells[type]; id++)
      {

        cellId = sendCellList[type][id];
        for (i = prevCellId; i < cellId; i++)
        {
          // ... increment pointers to get to correct starting
          //   point ...
          npts = *inPtr++;
          inPtr += npts;
        }
        prevCellId = cellId + 1;

        npts = *inPtr++;
        *ptr++ = npts;
        ptcntr[type]++;

        for (i = 0; i < npts; i++)
        {
          pointId = *inPtr++;
          if (usedIds[pointId] == -1)
          {
            vtkIdType newPt = pointIncr;
            *ptr++ = newPt;
            ptcntr[type]++;
            usedIds[pointId] = newPt;
            fromPtIds[pointIncr] = pointId;
            pointIncr++;
          }
          else
          {
            // ... use new point id ...
            *ptr++ = usedIds[pointId];
            ptcntr[type]++;
          }
        } // end loop over npts
      }   // end loop over numCells
    }     // end else where sendCellList isn't null
  }       // end of type loop

  if (numPoints != pointIncr)
  {
    vtkErrorMacro("numPoints=" << numPoints << ", pointIncr=" << pointIncr << ", should be equal");
  }

  delete[] usedIds;

  // ... send cell data attribute data (Scalars, Vectors, etc.)...

  vtkCellData* inputCellData = input->GetCellData();
  vtkCellData* outputCellData = output->GetCellData();

  vtkIdType cellOffset = 0;
  vtkIdType inputNumCells;

  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    vtkIdType cnt = 0;
    vtkIdType* fromIds = new vtkIdType[numCells[type]];
    if (sendCellList != NULL)
    {
      for (cellId = startCell[type]; cellId <= stopCell[type]; cellId++)
      {
        fromIds[cnt] = sendCellList[cnt][type] + cellOffset;
        cnt++;
      }
    }

    // ... output needed for flags only (assumes flags are the same
    //   on all processors) ...

    int typetag = type; //(typetag = type for cells, =5 for points)
    if (sendCellList == NULL)
    {
      this->SendCellBlockDataArrays(inputCellData, outputCellData, numCells[type], sendTo,
        startCell[type] + cellOffset, typetag);
    }
    else
    {
      this->SendDataArrays(inputCellData, outputCellData, numCells[type], sendTo, fromIds, typetag);
    }

    inputNumCells = 0;
    if (inputCellArrays[type])
    {
      inputNumCells = inputCellArrays[type]->GetNumberOfCells();
    }
    cellOffset += inputNumCells;

    delete[] fromIds; // this array was allocated above in
    // this case
  }

  // ... Send points Id's in cells now ...

  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (ptcntr[type] > 0)
    {
      this->Controller->Send(ptrsav[type], ptcntr[type], sendTo, CELL_TAG + type);
    }
  }

  // ... Copy cell points. ...

  vtkPoints* inputPoints = input->GetPoints();
  vtkDataArray* inputPointsArray = inputPoints->GetData();
  void* inputPointsArrayData = inputPointsArray->GetVoidPointer(0);

  float* outputPointsArrayData = new float[3 * numPoints];

  // ... send x,y, z coordinates of points

  int j;
  vtkIdType inLoc, outLoc;
  switch (inputPointsArray->GetDataType())
  {
    vtkTemplateMacro(for (i = 0; i < numPoints; i++) {
      inLoc = fromPtIds[i] * 3;
      outLoc = i * 3;
      for (j = 0; j < 3; j++)
      {
        outputPointsArrayData[outLoc + j] =
          static_cast<float>(reinterpret_cast<VTK_TT*>(inputPointsArrayData)[inLoc + j]);
      }
    });
  }

  // ... Send points now ...

  this->Controller->Send(outputPointsArrayData, 3 * numPoints, sendTo, POINTS_TAG);

  // ... use output for flags only to avoid unnecessary sends ...
  vtkPointData* inputPointData = input->GetPointData();
  vtkPointData* outputPointData = output->GetPointData();

  int typetag = 5; //(typetag = 0 for cells + type, =5 for points)

  this->SendDataArrays(inputPointData, outputPointData, numPoints, sendTo, fromPtIds, typetag);
  delete[] fromPtIds;
}
//****************************************************************
void vtkRedistributePolyData::ReceiveCells(vtkIdType* startCell, vtkIdType* stopCell,
  vtkPolyData* output, int recFrom, vtkIdType* prevCellptCntr, vtkIdType* cellptCntr,
  vtkIdType prevNumPoints, vtkIdType numPoints)

//*****************************************************************
{
  // ... send cells, points and associated data from cells in
  //     specified region ...

  vtkIdType cellId, i;

  // ... receive cell data attribute data (Scalars, Vectors, etc.)...

  vtkIdType cellOffset = 0;

  vtkCellData* outputCellData = output->GetCellData();

  vtkCellArray* outputCellArrays[NUM_CELL_TYPES];
  outputCellArrays[0] = output->GetVerts();
  outputCellArrays[1] = output->GetLines();
  outputCellArrays[2] = output->GetPolys();
  outputCellArrays[3] = output->GetStrips();

  int type;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    vtkIdType cnt = 0;
    vtkIdType numCells = stopCell[type] - startCell[type] + 1;
    vtkIdType* toIds = new vtkIdType[numCells];
    for (cellId = startCell[type]; cellId <= stopCell[type]; cellId++)
    {
      toIds[cnt++] = cellId + cellOffset;
    }

    int typetag = type; //(typetag = type for cells, =5 for points)
    this->ReceiveDataArrays(outputCellData, numCells, recFrom, toIds, typetag);
    delete[] toIds;

    vtkIdType outputNumCells = 0;
    if (outputCellArrays[type])
    {
      outputNumCells = outputCellArrays[type]->GetNumberOfCells();
    }
    cellOffset += outputNumCells;
  }

  // ... receive point Id's for all the points in the cell. ...

  vtkIdType* outPtr;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (outputCellArrays[type])
    {
      outPtr = outputCellArrays[type]->GetPointer();
      outPtr += prevCellptCntr[type];

      if (cellptCntr[type] && outPtr)
      {
        this->Controller->Receive((vtkIdType*)outPtr, cellptCntr[type], recFrom, CELL_TAG + type);
      }

      // ... Fix pointId's (need to have offset added to represent
      //   correct location ...

      for (cellId = startCell[type]; cellId <= stopCell[type]; cellId++)
      {
        vtkIdType npts = *outPtr++;
        for (i = 0; i < npts; i++)
        {
          *outPtr += prevNumPoints;
          outPtr++;
        }
      }
    } // end if outputCellArrays[type]
  }   // end loop over type

  // ... Receive points now ...

  vtkPoints* outputPoints = output->GetPoints();
  vtkFloatArray* outputPointsArray = vtkFloatArray::SafeDownCast(outputPoints->GetData());
  float* outputPointsArrayData = outputPointsArray->GetPointer(0);

  this->Controller->Receive(
    &outputPointsArrayData[prevNumPoints * 3], 3 * numPoints, recFrom, POINTS_TAG);

  // ... receive point attribute data ...
  vtkIdType* toPtIds = new vtkIdType[numPoints];
  for (i = 0; i < numPoints; i++)
  {
    toPtIds[i] = prevNumPoints + i;
  }

  vtkPointData* outputPointData = output->GetPointData();
  int typetag = 5; //(typetag = type for cells, =5 for points)
  this->ReceiveDataArrays(outputPointData, numPoints, recFrom, toPtIds, typetag);
  delete[] toPtIds;
}
//*******************************************************************
// Allocate space for the attribute data expected from all id's.

void vtkRedistributePolyData::AllocatePointDataArrays(
  vtkDataSetAttributes* toPd, vtkIdType* numPtsToCopy, int cntRec, vtkIdType numPtsToCopyOnProc)
{
  vtkIdType numPtsToCopyTotal = numPtsToCopyOnProc;
  int id;
  for (id = 0; id < cntRec; id++)
    numPtsToCopyTotal += numPtsToCopy[id];

  // ... Use WritePointer to allocate memory because it copies
  //   existing data and only allocates if necessary. ...

  vtkDataArray* Data;
  int numArrays = toPd->GetNumberOfArrays();

  for (int i = 0; i < numArrays; i++)
  {
    Data = toPd->GetArray(i);

    this->AllocateArrays(Data, numPtsToCopyTotal);
  }
}
//*******************************************************************
// Allocate space for the attribute data expected from all id's.

void vtkRedistributePolyData::AllocateCellDataArrays(vtkDataSetAttributes* toPd,
  vtkIdType** numCellsToCopy, int cntRec, vtkIdType* numCellsToCopyOnProc)
{
  int type;
  vtkIdType numCellsToCopyTotal = 0;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    numCellsToCopyTotal += numCellsToCopyOnProc[type];

    int id;
    for (id = 0; id < cntRec; id++)
    {
      numCellsToCopyTotal += numCellsToCopy[type][id];
    }
  }

  vtkDataArray* Data;
  int numArrays = toPd->GetNumberOfArrays();

  for (int i = 0; i < numArrays; i++)
  {
    Data = toPd->GetArray(i);

    this->AllocateArrays(Data, numCellsToCopyTotal);
  }
}
//*******************************************************************
void vtkRedistributePolyData::AllocateArrays(vtkDataArray* Data, vtkIdType numToCopyTotal)
//****************************************************************
{
  // loses some checks for "invalid" types like bit and unsigned short
  int numComp = Data->GetNumberOfComponents();

  if (numToCopyTotal > 0)
  {
    if (Data->WriteVoidPointer(0, numToCopyTotal * numComp) == 0)
    {
      vtkErrorMacro("Error: can't alloc mem for data array");
    }
  } // end of if numToCopyTotal>0
}
//----------------------------------------------------------------------
//*****************************************************************
void vtkRedistributePolyData::FindMemReq(
  vtkIdType* origNumCells, vtkPolyData* input, vtkIdType& numPoints, vtkIdType* numCellPts)
//*****************************************************************
{
  // ... count number of cellpoints, corresponding points and
  //   number of cells ...
  vtkIdType cellId, i;

  // ... Allocate maximum possible number of points (use total from
  //     all of input) ...

  vtkIdType numPointsMax = input->GetNumberOfPoints();
  vtkIdType* usedIds = new vtkIdType[numPointsMax];
  for (i = 0; i < numPointsMax; i++)
    usedIds[i] = -1;

  // ... count point Id's for all the points in the cell
  //     and number of points that will be stored ...

  vtkIdType pointId;

  vtkCellArray* cellArrays[NUM_CELL_TYPES];
  cellArrays[0] = input->GetVerts();
  cellArrays[1] = input->GetLines();
  cellArrays[2] = input->GetPolys();
  cellArrays[3] = input->GetStrips();

  numPoints = 0;

  vtkIdType* inPtr;

  int type;
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (cellArrays[type])
    {
      inPtr = cellArrays[type]->GetPointer();
      numCellPts[type] = 0;
      for (cellId = 0; cellId < origNumCells[type]; cellId++)
      {
        vtkIdType npts = *inPtr++;
        numCellPts[type]++;
        numCellPts[type] += npts;
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
    }
  }

  delete[] usedIds;
}

//*****************************************************************
//*****************************************************************
// Copy the attribute data from one id to another. Make sure CopyAllocate() has// been invoked
// before using this method.
void vtkRedistributePolyData::SendDataArrays(vtkDataSetAttributes* fromPd,
  vtkDataSetAttributes* vtkNotUsed(toPd), vtkIdType numToCopy, int sendTo, vtkIdType* fromId,
  int typetag)
{

  vtkDataArray* Data;
  int numArrays = fromPd->GetNumberOfArrays();

  // Note: sendTag is just mpi tag to keep sends separate
  int sendTag;

  for (int i = 0; i < numArrays; i++)
  {
    Data = fromPd->GetArray(i);

    sendTag = 200 + 10 * i + typetag; // these tags should be unique
    SendArrays(Data, numToCopy, sendTo, fromId, sendTag);
  }
}
//*****************************************************************
// Copy the attribute data from one id to another. Make sure
// CopyAllocate() has// been invoked before using this method.

void vtkRedistributePolyData::SendCellBlockDataArrays(vtkDataSetAttributes* fromPd,
  vtkDataSetAttributes* vtkNotUsed(toPd), vtkIdType numToCopy, int sendTo, vtkIdType startCell,
  int typetag)
//*******************************************************************
{

  vtkDataArray* Data;
  int numArrays = fromPd->GetNumberOfArrays();

  for (int i = 0; i < numArrays; i++)
  {
    Data = fromPd->GetArray(i);

    int sendTag = 200 + 10 * i + typetag; // these tags should be
    // unique
    this->SendBlockArrays(Data, numToCopy, sendTo, startCell, sendTag);
  }
}
//------------------------------------------------------------------
namespace
{
template <typename T>
void SendArraysTemplate(vtkDataArray* Data, vtkIdType numToCopy, int sendTo, vtkIdType* fromId,
  int sendTag, vtkMultiProcessController* controller)
{
  T* array = (T*)Data->GetVoidPointer(0);
  int numComps = Data->GetNumberOfComponents();
  T* buf = new T[numToCopy * numComps];
  vtkIdType size = numToCopy * numComps;
  for (vtkIdType i = 0; i < numToCopy; ++i)
  {
    for (int j = 0; j < numComps; ++j)
    {
      buf[numComps * i + j] = array[numComps * fromId[i] + j];
    }
  }
  controller->Send(buf, size, sendTo, sendTag);
  delete[] buf;
}
}
//******************************************************************
void vtkRedistributePolyData::SendArrays(
  vtkDataArray* Data, vtkIdType numToCopy, int sendTo, vtkIdType* fromId, int sendTag)
//******************************************************************
{
  int dataType = Data->GetDataType();

  switch (dataType)
  {
    vtkTemplateMacro(
      SendArraysTemplate<VTK_TT>(Data, numToCopy, sendTo, fromId, sendTag, this->Controller));
    case VTK_BIT:
      vtkErrorMacro("VTK_BIT not allowed for send");
      break;
    default:
      vtkErrorMacro("datatype = " << dataType << " not allowed for send");
  }
}
//-----------------------------------------------------------------
namespace
{
template <typename T>
void SendBlockArraysTemplate(vtkDataArray* Data, vtkIdType numToCopy, int sendTo,
  vtkIdType startCell, int sendTag, vtkMultiProcessController* controller)
{
  int numComps = Data->GetNumberOfComponents();

  vtkIdType start = numComps * startCell;
  vtkIdType size = numToCopy * numComps;

  T* array = (T*)Data->GetVoidPointer(0);
  controller->Send((T*)&array[start], size, sendTo, sendTag);
}
}
//******************************************************************
void vtkRedistributePolyData::SendBlockArrays(
  vtkDataArray* Data, vtkIdType numToCopy, int sendTo, vtkIdType startCell, int sendTag)
//******************************************************************
{
  int dataType = Data->GetDataType();

  // again adds support for unsigned short...
  switch (dataType)
  {
    vtkTemplateMacro(SendBlockArraysTemplate<VTK_TT>(
      Data, numToCopy, sendTo, startCell, sendTag, this->Controller));
    case VTK_BIT:
      vtkErrorMacro("VTK_BIT not allowed for send");
      break;
    default:
      vtkErrorMacro("datatype = " << dataType << " not allowed for send");
  }
}
//*****************************************************************
// ... Receive the attribute data from recFrom.  Call
//   AllocateDataArrays before calling this ...

void vtkRedistributePolyData::ReceiveDataArrays(
  vtkDataSetAttributes* toPd, vtkIdType numToCopy, int recFrom, vtkIdType* toId, int typetag)
{

  // ... this assumes that memory has been allocated already, this is
  //     helpful to avoid repeatedly resizing ...

  vtkDataArray* Data;
  int numArrays = toPd->GetNumberOfArrays();

  // Note: recTag is just mpi tag to keep receives separate
  int recTag;

  for (int i = 0; i < numArrays; i++)
  {
    Data = toPd->GetArray(i);

    recTag = 200 + 10 * i + typetag; // these tags should be unique
    this->ReceiveArrays(Data, numToCopy, recFrom, toId, recTag);
  }
}
//-------------------------------------------------------------------
namespace
{
template <typename T>
void ReceiveArraysTemplate(vtkDataArray* Data, vtkIdType numToCopy, int recFrom, vtkIdType* toId,
  int recTag, vtkMultiProcessController* controller, bool setDataToRecFrom)
{
  int numComps = Data->GetNumberOfComponents();

  T* array = (T*)Data->GetVoidPointer(0);
  T* buf = new T[numToCopy * numComps];

  controller->Receive(buf, numToCopy * numComps, recFrom, recTag);
  if (setDataToRecFrom)
  {
    for (vtkIdType i = 0; i < numToCopy; ++i)
    {
      for (int j = 0; j < numComps; ++j)
      {
        array[toId[i] * numComps + j] = recFrom;
      }
    }
  }
  else
  {
    for (vtkIdType i = 0; i < numToCopy; ++i)
    {
      for (int j = 0; j < numComps; ++j)
      {
        array[toId[i] * numComps + j] = buf[numComps * i + j];
      }
    }
  }
  delete[] buf;
}
}
//*******************************************************************
void vtkRedistributePolyData::ReceiveArrays(
  vtkDataArray* Data, vtkIdType numToCopy, int recFrom, vtkIdType* toId, int recTag)
//*******************************************************************
{
  int dataType = Data->GetDataType();

  bool setDataToRecFrom = (dataType == VTK_DOUBLE) && this->ColorProc;

  switch (dataType)
  {
    vtkTemplateMacro(ReceiveArraysTemplate<VTK_TT>(
      Data, numToCopy, recFrom, toId, recTag, this->Controller, setDataToRecFrom));
    case VTK_BIT:
      vtkErrorMacro("VTK_BIT not allowed for receive");
      break;
    default:
      vtkErrorMacro("datatype = " << dataType << " not allowed for receive");
  }
}

//--------------------------------------------------------------------
int vtkRedistributePolyData::DoubleCheckArrays(vtkPolyData* input)
{
  int mismatch = 0;
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  // Sanity check: Avoid haning on bad input.
  // Format a message that has all of the array information.
  // All arrays must be the same type and numcomps on all procs.
  // This keeps us from locking up.
  int length = input->GetPointData()->GetNumberOfArrays();
  length += input->GetCellData()->GetNumberOfArrays();
  // num PD arrays, num CD arrays, type and num comps per array.
  length = 2 + length * 2;
  int* sanity = new int[length];
  int idx, numArrays, count;
  vtkDataArray* array;
  sanity[0] = input->GetPointData()->GetNumberOfArrays();
  sanity[1] = input->GetCellData()->GetNumberOfArrays();
  count = 2;
  numArrays = sanity[0];
  for (idx = 0; idx < numArrays; ++idx)
  {
    array = input->GetPointData()->GetArray(idx);
    sanity[count++] = array->GetDataType();
    sanity[count++] = array->GetNumberOfComponents();
  }
  numArrays = sanity[1];
  for (idx = 0; idx < numArrays; ++idx)
  {
    array = input->GetCellData()->GetArray(idx);
    sanity[count++] = array->GetDataType();
    sanity[count++] = array->GetNumberOfComponents();
  }
  if (myId == 0)
  {
    // Broadcast my info as the correct arrays.
    for (idx = 1; idx < numProcs; ++idx)
    {
      this->Controller->Send(&length, 1, idx, 77431);
      this->Controller->Send(sanity, length, idx, 77432);
    }
    // Receive matches
    int otherMismatch;
    for (idx = 1; idx < numProcs; ++idx)
    {
      this->Controller->Receive(&otherMismatch, 1, idx, 77433);
      if (otherMismatch)
      {
        mismatch = 1;
      }
    }
    // Send out the final mismatch result to all procs.
    for (idx = 1; idx < numProcs; ++idx)
    {
      this->Controller->Send(&mismatch, 1, idx, 77434);
    }
  }
  else
  {
    int zeroLength;
    int* zeroSanity;
    this->Controller->Receive(&zeroLength, 1, 0, 77431);
    zeroSanity = new int[zeroLength];
    this->Controller->Receive(zeroSanity, zeroLength, 0, 77432);

    if (input->GetNumberOfPoints() == 0 && input->GetNumberOfCells() == 0)
    {
      // an empty dataset on a processor with Id>0 does not mismatch.
      mismatch = 0;
    }
    else
    {
      // Compare
      if (length != zeroLength)
      {
        mismatch = 1;
      }
      else
      {
        for (idx = 0; idx < length; ++idx)
        {
          if (sanity[idx] != zeroSanity[idx])
          {
            mismatch = 1;
          }
        }
      }
    }
    delete[] zeroSanity;
    zeroSanity = NULL;

    this->Controller->Send(&mismatch, 1, 0, 77433);
    this->Controller->Receive(&mismatch, 1, 0, 77434);
  }

  delete[] sanity;
  sanity = NULL;
  if (mismatch)
  {
    return 0;
  }

  return 1;
}

//--------------------------------------------------------------------
// I am using no points as the indicator that arrays need completion.
// It is possible that no cells could also cause trouble.
void vtkRedistributePolyData::CompleteInputArrays(vtkPolyData* input)
{
  if (this->Controller == NULL)
  {
    vtkErrorMacro("Missing controller.");
    return;
  }

  int idx;
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int* msg = new int[numProcs];
  int numPts = input->GetNumberOfPoints();
  if (myId > 0)
  {
    // First send the number of points to process zero.
    this->Controller->Send(&numPts, 1, 0, 87873);
    // Just a broadcast.  Receive num points from all procs.
    this->Controller->Receive(msg, numProcs, 0, 87874);
  }
  else
  {
    msg[0] = numPts;
    for (idx = 1; idx < numProcs; ++idx)
    {
      this->Controller->Receive(&numPts, 1, idx, 87873);
      msg[idx] = numPts;
    }
    for (idx = 1; idx < numProcs; ++idx)
    {
      this->Controller->Send(msg, numProcs, idx, 87874);
    }
  }

  // Who is sending?
  int sendProc = -1;
  for (idx = 0; idx < numProcs; ++idx)
  {
    if (msg[idx] > 0)
    {
      sendProc = idx;
    }
  }
  if (sendProc == -1)
  { // No proc has data.
    delete[] msg;
    return;
  }
  if (myId == sendProc)
  {
    for (idx = 0; idx < numProcs; ++idx)
    {
      if (msg[idx] == 0)
      {
        this->SendInputArrays(input->GetPointData(), idx);
        this->SendInputArrays(input->GetCellData(), idx);
      }
    }
  }

  if (msg[myId] == 0)
  {
    this->ReceiveInputArrays(input->GetPointData(), sendProc);
    this->ReceiveInputArrays(input->GetCellData(), sendProc);
  }
}

//--------------------------------------------------------------------
void vtkRedistributePolyData::ReceiveInputArrays(vtkDataSetAttributes* attr, int recFrom)
{
  int j;
  int num = 0;
  vtkDataArray* array = 0;
  char* name;
  int nameLength = 0;
  int type = 0;
  int numComps = 0;
  int index = -1;
  int attributeType = 0;
  int copyFlag = 0;

  attr->Initialize();

  this->Controller->Receive(&num, 1, recFrom, 997244);
  for (j = 0; j < num; ++j)
  {
    this->Controller->Receive(&type, 1, recFrom, 997245);
    {
      vtkAbstractArray* TempArray = vtkAbstractArray::CreateArray(type);
      array = vtkDataArray::SafeDownCast(TempArray);
      if (!array)
      {
        TempArray->Delete();
      }
    }
    this->Controller->Receive(&numComps, 1, recFrom, 997246);
    this->Controller->Receive(&nameLength, 1, recFrom, 997247);
    if (array)
    {
      array->SetNumberOfComponents(numComps);
      if (nameLength > 0)
      {
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, recFrom, 997248);
        array->SetName(name);
        delete[] name;
        name = NULL;
      }
      else
      {
        array->SetName(NULL);
      }
      index = attr->AddArray(array);
      array->Delete();
      array = NULL;
    }
    this->Controller->Receive(&attributeType, 1, recFrom, 997249);
    this->Controller->Receive(&copyFlag, 1, recFrom, 997250);

    if (attributeType != -1 && copyFlag)
    {
      attr->SetActiveAttribute(index, attributeType);
    }
  } // end of loop over arrays.
}

//-----------------------------------------------------------------------
void vtkRedistributePolyData::SendInputArrays(vtkDataSetAttributes* attr, int sendTo)
{
  int num;
  int i;
  int type;
  int numComps;
  int nameLength;
  const char* name;
  vtkDataArray* array;
  int attributeType;
  int copyFlag;

  num = attr->GetNumberOfArrays();
  this->Controller->Send(&num, 1, sendTo, 997244);
  for (i = 0; i < num; ++i)
  {
    array = attr->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, sendTo, 997245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, sendTo, 997246);
    name = array->GetName();
    if (name == NULL)
    {
      nameLength = 0;
    }
    else
    {
      nameLength = (int)strlen(name) + 1;
    }
    this->Controller->Send(&nameLength, 1, sendTo, 997247);
    if (nameLength > 0)
    {
      // I am pretty sure that Send does not modify the string.
      this->Controller->Send(const_cast<char*>(name), nameLength, sendTo, 997248);
    }

    attributeType = attr->IsArrayAnAttribute(i);
    copyFlag = -1;
    if (attributeType != -1)
    {
      // ... Note: this would be much simpler if there was a
      //    GetCopyAttributeFlag function or if the variable
      //    wasn't protected. ...
      switch (attributeType)
      {
        case vtkDataSetAttributes::SCALARS:
          copyFlag = attr->GetCopyScalars();
          break;

        case vtkDataSetAttributes::VECTORS:
          copyFlag = attr->GetCopyVectors();
          break;

        case vtkDataSetAttributes::NORMALS:
          copyFlag = attr->GetCopyNormals();
          break;

        case vtkDataSetAttributes::TCOORDS:
          copyFlag = attr->GetCopyTCoords();
          break;

        case vtkDataSetAttributes::TENSORS:
          copyFlag = attr->GetCopyTensors();
          break;

        default:
          copyFlag = 0;
      }
    }
    this->Controller->Send(&attributeType, 1, sendTo, 997249);
    this->Controller->Send(&copyFlag, 1, sendTo, 997250);
  }
}

//=============================================================

vtkRedistributePolyData::vtkCommSched::vtkCommSched()
{
  // ... initialize a communication schedule to do nothing ...
  this->NumberOfCells = 0;
  this->SendCount = 0;
  this->ReceiveCount = 0;
  this->SendTo = NULL;
  this->SendNumber = NULL;
  this->ReceiveFrom = NULL;
  this->ReceiveNumber = NULL;
  this->SendCellList = NULL;
  this->KeepCellList = NULL;
}

//*****************************************************************
vtkRedistributePolyData::vtkCommSched::~vtkCommSched()
{
  delete[] this->SendTo;
  delete[] this->ReceiveFrom;

  int type;

  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (this->SendNumber != NULL)
    {
      delete[] this->SendNumber[type];
    }
    if (this->ReceiveNumber != NULL)
    {
      delete[] this->ReceiveNumber[type];
    }

    if (this->SendCellList != NULL)
    {
      for (int i = 0; i < this->SendCount; i++)
      {
        delete[] this->SendCellList[i][type];
      }
    }
    if (this->KeepCellList != NULL)
    {
      delete[] this->KeepCellList[type];
    }
  }

  if (this->SendCellList != NULL)
  {
    for (int i = 0; i < this->SendCount; i++)
    {
      delete[] this->SendCellList[i];
    }
    delete[] this->SendCellList;
  }

  delete[] this->SendNumber;
  delete[] this->ReceiveNumber;
  delete[] this->KeepCellList;
  delete[] this->NumberOfCells;
}
//*****************************************************************
