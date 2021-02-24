/*=========================================================================

  Program:   ParaView
  Module:    vtkWeightedRedistributePolyData.cxx

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

#include "vtkWeightedRedistributePolyData.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#define NUM_CELL_TYPES 4

//-------------------------------------------------------------------
vtkStandardNewMacro(vtkWeightedRedistributePolyData);

//-------------------------------------------------------------------

vtkWeightedRedistributePolyData::vtkWeightedRedistributePolyData()
{
  this->Weights = nullptr;
}

//-------------------------------------------------------------------

vtkWeightedRedistributePolyData::~vtkWeightedRedistributePolyData()
{
  delete[] Weights;
}

//-------------------------------------------------------------------

void vtkWeightedRedistributePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRedistributePolyData::PrintSelf(os, indent);
}

//*****************************************************************
void vtkWeightedRedistributePolyData::SetWeights(int startProc, int stopProc, float weight)
{
  int myId, numProcs;
  if (!this->Controller)
  {
    vtkErrorMacro("need controller to set weights");
    return;
  }
  numProcs = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();

  // ... Only set weights on processor 0 to avoid extra
  //  communication. ...
  if (myId == 0)
  {
    int np;
    if (Weights == nullptr)
    {
      Weights = new float[numProcs];
      for (np = 0; np < numProcs; np++)
      {
        Weights[np] = 1.;
      }
    }
    for (np = startProc; np <= stopProc; np++)
    {
      Weights[np] = weight;
    }
  }
}

//*****************************************************************
void vtkWeightedRedistributePolyData::MakeSchedule(vtkPolyData* input, vtkCommSched* localSched)

{
  //*****************************************************************
  // purpose: This routine sets up a schedule to shift cells around so
  //          the number of cells on each processor is as even as possible.
  //
  //*****************************************************************

  // get total number of polys and figure out how many each processor should have

  int type;
  vtkCellArray* inputCellArrays[NUM_CELL_TYPES];
  inputCellArrays[0] = input->GetVerts();
  inputCellArrays[1] = input->GetLines();
  inputCellArrays[2] = input->GetPolys();
  inputCellArrays[3] = input->GetStrips();

  vtkIdType numLocalCells[NUM_CELL_TYPES];
  for (type = 0; type < NUM_CELL_TYPES; type++)
  {
    if (inputCellArrays[type])
    {
      numLocalCells[type] = inputCellArrays[type]->GetNumberOfCells();
    }
    else
    {
      numLocalCells[type] = 0;
    }
  }

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
    if (Weights == nullptr)
    {
      // ... no weights have been set so this is a uniform balance
      //  case ...
      Weights = new float[numProcs];
      for (id = 0; id < numProcs; id++)
      {
        Weights[id] = 1. / numProcs;
      }
    }
    else
    {
      // ... weights have been set so normalize to add to 1. ...
      float weight_sum = 0.;
      for (id = 0; id < numProcs; id++)
      {
        weight_sum += Weights[id];
      }

      float weight_scale = 0.;
      if (weight_sum > 0)
        weight_scale = 1. / weight_sum;
      for (id = 0; id < numProcs; id++)
      {
        Weights[id] *= weight_scale;
      }
    }
  }

  vtkCommSched* remoteSched = new vtkCommSched[numProcs];

  vtkIdType totalCells[NUM_CELL_TYPES];
  vtkIdType numRemoteCells[NUM_CELL_TYPES];
  vtkIdType* goalNumCells[NUM_CELL_TYPES];
  vtkIdType leftovers[NUM_CELL_TYPES];
  int numProcZero[NUM_CELL_TYPES];
  for (type = 0; type < NUM_CELL_TYPES; type++)
    numProcZero[type] = 0;
  if (myId != 0)
  {
    this->Controller->Send((vtkIdType*)(numLocalCells), NUM_CELL_TYPES, 0, NUM_LOC_CELLS_TAG);
  }
  else
  {
    remoteSched[0].NumberOfCells = new vtkIdType[NUM_CELL_TYPES];
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      totalCells[type] = numLocalCells[type];
      remoteSched[0].NumberOfCells[type] = numLocalCells[type];
    }
    for (id = 1; id < numProcs; id++)
    {
      this->Controller->Receive(
        (vtkIdType*)(&numRemoteCells), NUM_CELL_TYPES, id, NUM_LOC_CELLS_TAG);
      remoteSched[id].NumberOfCells = new vtkIdType[NUM_CELL_TYPES];
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        totalCells[type] += numRemoteCells[type];
        remoteSched[id].NumberOfCells[type] = numRemoteCells[type];
      }
    }

    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      float weightTotalOfRemainingProcesses = 1.0;
      vtkIdType numCellsLeftToDivideUp = totalCells[type];
      goalNumCells[type] = new vtkIdType[numProcs];
      for (id = 0; id < numProcs; id++)
      {
        // Round here instead of truncating.
        if (weightTotalOfRemainingProcesses > 0)
        {
          goalNumCells[type][id] =
            static_cast<vtkIdType>((static_cast<float>(numCellsLeftToDivideUp) *
                                     (Weights[id] / weightTotalOfRemainingProcesses)) +
              0.5);
        }
        else
        {
          goalNumCells[type][id] = 0;
        }
        numCellsLeftToDivideUp -= goalNumCells[type][id];
        weightTotalOfRemainingProcesses -= Weights[id];

        // Put this back until I can fix the tests.
        // goalNumCells[type][id] =
        //  static_cast<vtkIdType>(numCellsLeftToDivideUp * Weights[id]);
        if (goalNumCells[type][id] == 0)
        {
          numProcZero[type]++;
        }
      }
    }

    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      leftovers[type] = 0;
      vtkIdType sumCells = 0;
      for (id = 0; id < numProcs; id++)
      {
        sumCells += goalNumCells[type][id];
      }
      leftovers[type] = totalCells[type] - sumCells;
      // sanity check before I get rid of this variable.
      if (leftovers[type] != 0)
      {
        vtkErrorMacro("non zero leftover[" << type << "] = " << leftovers[type]);
        leftovers[type] = 0;
      }
    }
  }

  // ... order processors to minimize the number of sends (replace
  //  with more efficient sort later) ...
  int schedLen1;
  int schedLen2;
  int* schedArray1;
  vtkIdType* schedArray2;

  int temp;
  if (myId == 0)
  {
    for (id = 0; id < numProcs; id++)
    {
      remoteSched[id].SendCount = 0;
    }

    // ... create temporary variable that will be stored in the
    //  remoteSchedules after reordering and compiling all types ...
    int* rsCntSend[NUM_CELL_TYPES];
    int** rsSendTo[NUM_CELL_TYPES];
    vtkIdType** rsSendNum[NUM_CELL_TYPES];

    int* sendToTemp = nullptr;
    vtkIdType* sendNumTemp = nullptr;
    sendToTemp = new int[numProcs];
    sendNumTemp = new vtkIdType[numProcs];

    int id2;
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      int* order = new int[numProcs];
      for (id = 0; id < numProcs; id++)
      {
        order[id] = id;
      }
      for (id = 0; id < numProcs; id++)
      {
        for (id2 = id + 1; id2 < numProcs; id2++)
        {
          if ((remoteSched[order[id]].NumberOfCells[type] - goalNumCells[type][order[id]]) <
            (remoteSched[order[id2]].NumberOfCells[type] - goalNumCells[type][order[id2]]))
          {
            temp = order[id];
            order[id] = order[id2];
            order[id2] = temp;
          }
        }
      }

      // now figure out what processors to send cells between
      vtkIdType numToSend, numToReceive;

      int start = 0;
      int last = numProcs - 1;
      int cnt = 0;

      int recflag = 0; // turn on if receive should get extra
                       // leftover cell, without flag this gets lost

      rsCntSend[type] = new int[numProcs];
      rsSendTo[type] = new int*[numProcs];
      rsSendNum[type] = new vtkIdType*[numProcs];

      for (id = 0; id < numProcs; id++)
      {
        rsCntSend[type][id] = 0;
      }

      while (start < last)
      {
        int ostart = order[start];
        int olast = order[last];

        numToSend = remoteSched[ostart].NumberOfCells[type] - goalNumCells[type][ostart];

        // put in special test for when weights are exactly 0
        // to make sure all points are removed
        if ((leftovers[type]) > 0 && (goalNumCells[type][ostart] != 0))
        {
          numToSend--;
          leftovers[type]--;
        }

        cnt = 0;
        while (numToSend > 0)
        {
          if (start == last)
          {
            vtkErrorMacro("error: start =last");
          }
          numToReceive = goalNumCells[type][olast] - remoteSched[olast].NumberOfCells[type];

          if ((leftovers[type] >= (last - start - numProcZero[type]) && leftovers[type] > 0) ||
            (recflag == 1))
          {
            // ... receiving processors are going to get some of
            //  the leftover cells too ...
            numToReceive++;
            if (!recflag)
            {
              leftovers[type]--;
            } // if flag, this
            // has already been subtracted
            recflag = 1;
          }
          if (numToSend >= numToReceive)
          {
            sendToTemp[cnt] = olast;
            sendNumTemp[cnt++] = numToReceive;
            numToSend -= numToReceive;
            remoteSched[ostart].NumberOfCells[type] -= numToReceive;
            remoteSched[olast].NumberOfCells[type] += numToReceive;
            last--;
            olast = order[last];
            recflag = 0; // extra leftover point will have been
                         // used
          }
          else
          {
            sendToTemp[cnt] = olast;
            sendNumTemp[cnt++] = numToSend;
            remoteSched[ostart].NumberOfCells[type] -= numToSend;
            remoteSched[olast].NumberOfCells[type] += numToSend;
            numToSend = 0;
          }
        } // end while (numToSend>0)
        rsCntSend[type][ostart] = cnt;
        if (cnt > 0)
        {
          rsSendTo[type][ostart] = new int[cnt];
          rsSendNum[type][ostart] = new vtkIdType[cnt];
        }
        for (i = 0; i < cnt; i++)
        {
          rsSendTo[type][ostart][i] = sendToTemp[i];
          rsSendNum[type][ostart][i] = sendNumTemp[i];
        }

        start++;
      } // end while start<last loop
      delete[] order;
    } // end loop over type

    // ... combine results for all types ...

    for (id = 0; id < numProcs; id++)
    {
      // ... the number of processors sent to must be less than
      //  the sum over all types ...
      int maxCntSend = 0;
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        maxCntSend += rsCntSend[type][id];
      }
      int* tempSend = nullptr;
      if (maxCntSend > 0)
      {
        tempSend = new int[maxCntSend];
      }
      int j;

      int currSendCnt = 0;

      // ... now add other processors being sent by other types ..

      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        for (i = 0; i < rsCntSend[type][id]; i++)
        {
          int alreadyInList = 0;
          for (j = 0; j < currSendCnt; j++)
          {
            if (rsSendTo[type][id][i] == tempSend[j])
            {
              alreadyInList = 1;
              break;
            }
          }
          if (!alreadyInList)
          {
            tempSend[currSendCnt] = rsSendTo[type][id][i];
            currSendCnt++;
          } // end !already in list
        }   // end loop over rsCntSend
      }     // end loop over type
      remoteSched[id].SendCount = currSendCnt;
      if (currSendCnt > 0)
      {
        remoteSched[id].SendTo = new int[currSendCnt];
        remoteSched[id].SendNumber = new vtkIdType*[NUM_CELL_TYPES];
      }
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        if (currSendCnt > 0)
        {
          remoteSched[id].SendNumber[type] = new vtkIdType[currSendCnt];
        }
      }

      for (i = 0; i < currSendCnt; i++)
      {
        remoteSched[id].SendTo[i] = tempSend[i];

        for (type = 0; type < NUM_CELL_TYPES; type++)
        {
          // ... initialize to zero in case this type doesn't
          //  send any cells ...
          remoteSched[id].SendNumber[type][i] = 0;

          // ... now loop through all processors sent to
          //  by this type and if one is the same as the
          //  current one being sent to, change the number sent ...

          for (j = 0; j < rsCntSend[type][id]; j++)
          {
            if (rsSendTo[type][id][j] == tempSend[i])
            {
              remoteSched[id].SendNumber[type][i] = rsSendNum[type][id][j];
            }
          }
        } // end loop over type
      }   // end loop over number of processors processors sent to

      // ... clean up ...
      if (tempSend != nullptr)
      {
        delete[] tempSend;
      }
    } // end loop over procs

    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      for (id = 0; id < numProcs; id++)
      {
        if (rsCntSend[type][id] > 0)
        {
          delete[] rsSendTo[type][id];
          delete[] rsSendNum[type][id];
        }
      }
      delete[] rsCntSend[type];
      delete[] rsSendTo[type];
      delete[] rsSendNum[type];
    }

    for (id = 0; id < numProcs; id++)
    {
      remoteSched[id].ReceiveCount = 0;
    }
    // ... count up how processors a processor will be receiving from ...
    for (id = 0; id < numProcs; id++)
    {
      for (i = 0; i < remoteSched[id].SendCount; i++)
      {
        int sendId = remoteSched[id].SendTo[i];
        remoteSched[sendId].ReceiveCount++;
      }
    }

    // ... allocate memory to store the processor to receive from
    //  and how many to receive ...
    for (id = 0; id < numProcs; id++)
    {
      // remoteSched[id].ReceiveFrom =
      // new int [remoteSched[id].ReceiveCount];
      if (remoteSched[id].ReceiveCount > 0)
      {
        remoteSched[id].ReceiveFrom = new int[remoteSched[id].ReceiveCount];
        remoteSched[id].ReceiveNumber = new vtkIdType*[NUM_CELL_TYPES];
      }
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        if (remoteSched[id].ReceiveCount > 0)
        {
          remoteSched[id].ReceiveNumber[type] = new vtkIdType[remoteSched[id].ReceiveCount];
        }
      }
    }

    // ... reinitialize the count so records are stored in the
    //  correct place ...
    for (id = 0; id < numProcs; id++)
    {
      remoteSched[id].ReceiveCount = 0;
    }

    // ... store which processors will be received from and how
    //   many cells will be received ...
    for (id = 0; id < numProcs; id++)
    {
      for (i = 0; i < remoteSched[id].SendCount; i++)
      {
        int recId = remoteSched[id].SendTo[i];
        int remCntRec = remoteSched[recId].ReceiveCount;
        remoteSched[recId].ReceiveFrom[remCntRec] = id;
        for (type = 0; type < NUM_CELL_TYPES; type++)
        {
          remoteSched[recId].ReceiveNumber[type][remCntRec] = remoteSched[id].SendNumber[type][i];
        }
        remoteSched[recId].ReceiveCount++;
      }
    }

    for (id = 1; id < numProcs; id++)
    {
      // ... number of ints ...
      schedLen1 = 2 + remoteSched[id].SendCount + remoteSched[id].ReceiveCount;

      // ... number of vtkIdTypes ...
      schedLen2 = NUM_CELL_TYPES * (1 + remoteSched[id].SendCount + remoteSched[id].ReceiveCount);

      schedArray1 = new int[schedLen1];
      schedArray2 = new vtkIdType[schedLen2];
      for (type = 0; type < NUM_CELL_TYPES; type++)
        schedArray2[type] = remoteSched[id].NumberOfCells[type];
      schedArray1[0] = remoteSched[id].SendCount;
      schedArray1[1] = remoteSched[id].ReceiveCount;
      int arraycnt1 = 2;
      int arraycnt2 = NUM_CELL_TYPES;
      if (remoteSched[id].SendCount > 0)
      {
        for (i = 0; i < remoteSched[id].SendCount; i++)
        {
          schedArray1[arraycnt1++] = remoteSched[id].SendTo[i];
          for (type = 0; type < NUM_CELL_TYPES; type++)
          {
            schedArray2[arraycnt2++] = remoteSched[id].SendNumber[type][i];
          }
        }
      }
      if (remoteSched[id].ReceiveCount > 0)
      {
        for (i = 0; i < remoteSched[id].ReceiveCount; i++)
        {
          schedArray1[arraycnt1++] = remoteSched[id].ReceiveFrom[i];
          for (type = 0; type < NUM_CELL_TYPES; type++)
          {
            schedArray2[arraycnt2++] = remoteSched[id].ReceiveNumber[type][i];
          }
        }
      }
      this->Controller->Send((int*)(&schedLen1), 1, id, SCHED_LEN_1_TAG);
      this->Controller->Send((int*)(&schedLen2), 1, id, SCHED_LEN_2_TAG);
      this->Controller->Send((int*)schedArray1, schedLen1, id, SCHED_1_TAG);
      this->Controller->Send((vtkIdType*)schedArray2, schedLen2, id, SCHED_2_TAG);
      delete[] schedArray1;
      delete[] schedArray2;
    }

    // ... initialize the local schedule to return ...
    localSched->NumberOfCells = new vtkIdType[NUM_CELL_TYPES];
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      localSched->NumberOfCells[type] = remoteSched[0].NumberOfCells[type];
    }
    localSched->SendCount = remoteSched[0].SendCount;
    localSched->ReceiveCount = remoteSched[0].ReceiveCount;
    if (localSched->SendCount > 0)
    {
      localSched->SendTo = new int[localSched->SendCount];
      localSched->SendNumber = new vtkIdType*[NUM_CELL_TYPES];
    }
    if (localSched->ReceiveCount > 0)
    {
      localSched->ReceiveFrom = new int[localSched->ReceiveCount];
      localSched->ReceiveNumber = new vtkIdType*[NUM_CELL_TYPES];
    }
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      if (localSched->SendCount > 0)
      {
        localSched->SendNumber[type] = new vtkIdType[localSched->SendCount];
      }
      if (localSched->ReceiveCount > 0)
      {
        localSched->ReceiveNumber[type] = new vtkIdType[localSched->ReceiveCount];
      }
    }
    for (i = 0; i < localSched->SendCount; i++)
    {
      localSched->SendTo[i] = remoteSched[0].SendTo[i];
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        localSched->SendNumber[type][i] = remoteSched[0].SendNumber[type][i];
      }
    }
    for (i = 0; i < localSched->ReceiveCount; i++)
    {
      localSched->ReceiveFrom[i] = remoteSched[0].ReceiveFrom[i];
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        localSched->ReceiveNumber[type][i] = remoteSched[0].ReceiveNumber[type][i];
      }
    }

    delete[] sendToTemp;
    delete[] sendNumTemp;
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      delete[] goalNumCells[type];
    }
  }
  else
  {
    // myId != 0
    schedLen1 = 0;
    schedLen2 = 0;
    this->Controller->Receive((int*)(&schedLen1), 1, 0, SCHED_LEN_1_TAG);
    this->Controller->Receive((int*)(&schedLen2), 1, 0, SCHED_LEN_2_TAG);
    schedArray1 = new int[schedLen1];
    schedArray2 = new vtkIdType[schedLen2];
    this->Controller->Receive(schedArray1, schedLen1, 0, SCHED_1_TAG);
    this->Controller->Receive(schedArray2, schedLen2, 0, SCHED_2_TAG);

    localSched->NumberOfCells = new vtkIdType[NUM_CELL_TYPES];
    for (type = 0; type < NUM_CELL_TYPES; type++)
    {
      localSched->NumberOfCells[type] = schedArray2[type];
    }
    localSched->SendCount = schedArray1[0];
    localSched->ReceiveCount = schedArray1[1];
    int arraycnt1 = 2;
    int arraycnt2 = NUM_CELL_TYPES;
    if (localSched->SendCount > 0)
    {
      localSched->SendTo = new int[localSched->SendCount];
      localSched->SendNumber = new vtkIdType*[NUM_CELL_TYPES];
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        localSched->SendNumber[type] = new vtkIdType[localSched->SendCount];
      }
      for (i = 0; i < localSched->SendCount; i++)
      {
        localSched->SendTo[i] = schedArray1[arraycnt1++];
        for (type = 0; type < NUM_CELL_TYPES; type++)
        {
          localSched->SendNumber[type][i] = schedArray2[arraycnt2++];
        }
      }
    }
    if (localSched->ReceiveCount > 0)
    {
      localSched->ReceiveFrom = new int[localSched->ReceiveCount];
      localSched->ReceiveNumber = new vtkIdType*[NUM_CELL_TYPES];
      for (type = 0; type < NUM_CELL_TYPES; type++)
      {
        localSched->ReceiveNumber[type] = new vtkIdType[localSched->ReceiveCount];
      }
      for (i = 0; i < localSched->ReceiveCount; i++)
      {
        localSched->ReceiveFrom[i] = schedArray1[arraycnt1++];
        for (type = 0; type < NUM_CELL_TYPES; type++)
        {
          localSched->ReceiveNumber[type][i] = schedArray2[arraycnt2++];
        }
      }
    }
    delete[] schedArray1;
    delete[] schedArray2;
  }

  delete[] remoteSched;
}
