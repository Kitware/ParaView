/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaterialInterfacePieceTransactionMatrix.h"
#include "vtkCommunicator.h"
#include <cassert>
using std::vector;

namespace
{

#if 0 // Its usage is commented out below.
//-----------------------------------------------------------------------------
void IdentifyDownStreamRecipients(
      int child,
      int nProcs,
      vector<int> &DSR)
{
  DSR.push_back(child);

  // Process left child.
  child=2*child+1;
  if (child>=nProcs)
    {
    return;
    }
  IdentifyDownStreamRecipients(child,nProcs,DSR);
  // Process right child.
  ++child;
  if (child>=nProcs)
    {
    return;
    }
  IdentifyDownStreamRecipients(child,nProcs,DSR);
}
#endif
};

//-----------------------------------------------------------------------------
vtkMaterialInterfacePieceTransactionMatrix::vtkMaterialInterfacePieceTransactionMatrix()
{
  this->NFragments = 0;
  this->NProcs = 0;
  this->FlatMatrixSize = 0;
  this->Matrix = 0;
  this->NumberOfTransactions = 0;
}

//-----------------------------------------------------------------------------
vtkMaterialInterfacePieceTransactionMatrix::vtkMaterialInterfacePieceTransactionMatrix(
  int nFragments, int nProcs)
{
  this->Initialize(nFragments, nProcs);
}

//-----------------------------------------------------------------------------
vtkMaterialInterfacePieceTransactionMatrix::~vtkMaterialInterfacePieceTransactionMatrix()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfacePieceTransactionMatrix::Clear()
{
  this->NFragments = 0;
  this->NProcs = 0;
  if (this->Matrix)
  {
    delete[] this->Matrix;
    this->Matrix = 0;
  }
  this->NumberOfTransactions = 0;
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfacePieceTransactionMatrix::Initialize(int nFragments, int nProcs)
{
  this->Clear();

  this->NFragments = nFragments;
  this->NProcs = nProcs;
  this->FlatMatrixSize = nFragments * nProcs;
  this->Matrix = new vector<vtkMaterialInterfacePieceTransaction>[this->FlatMatrixSize];
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfacePieceTransactionMatrix::PushTransaction(
  int fragmentId, int procId, vtkMaterialInterfacePieceTransaction& transaction)
{
  int idx = fragmentId + procId * this->NFragments;
  this->Matrix[idx].push_back(transaction);
  ++this->NumberOfTransactions;
}

//-----------------------------------------------------------------------------
vtkIdType vtkMaterialInterfacePieceTransactionMatrix::Pack(int*& buf)
{
  /*
  the buffer has this structure:

  [nFragments,nProcs][nT,T0,...TN][nT,T0,...,TN]...[nT,T0,...,TN]...[nT,T0,...,TN]
                         /\           /\               /\               /\
                         |            |                |                |
                        f0,p0        f1,p0            fN,p0            fN,PN
  */

  // caller should not allocate memory.
  assert("Buffer appears to be pre-allocated." && buf == 0);

  const int transactionSize = vtkMaterialInterfacePieceTransaction::SIZE;

  const vtkIdType bufSize = this->FlatMatrixSize   // transaction count for each i,j
    + transactionSize * this->NumberOfTransactions // enough to store all transactions
    + 2;                                           // nFragments, nProcs

  buf = new int[bufSize];
  // header
  buf[0] = this->NFragments;
  buf[1] = this->NProcs;
  vtkIdType bufIdx = 2;

  for (int j = 0; j < this->NProcs; ++j)
  {
    for (int i = 0; i < this->NFragments; ++i)
    {
      int matIdx = i + j * this->NFragments;
      int nTransactions = static_cast<int>(this->Matrix[matIdx].size());

      // put the count for this i,j
      buf[bufIdx] = nTransactions;
      ++bufIdx;

      // put this i,j's transaction list
      for (int q = 0; q < nTransactions; ++q)
      {
        this->Matrix[matIdx][q].Pack(&buf[bufIdx]);
        bufIdx += transactionSize;
      }
    }
  }
  // now bufIdx is number of ints we have stored
  return bufIdx;
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfacePieceTransactionMatrix::UnPack(int* buf)
{
  assert("Buffer has not been allocated." && buf != 0);

  const int transactionSize = vtkMaterialInterfacePieceTransaction::SIZE;

  this->Initialize(buf[0], buf[1]);
  int bufIdx = 2;

  for (int j = 0; j < this->NProcs; ++j)
  {
    for (int i = 0; i < this->NFragments; ++i)
    {
      // get the number of transactions for this i,j
      int nTransactions = buf[bufIdx];
      ++bufIdx;

      // size the i,j th transaction list
      int matIdx = i + j * this->NFragments;
      this->Matrix[matIdx].resize(nTransactions);

      // load the i,j th transaction list
      for (int q = 0; q < nTransactions; ++q)
      {
        this->Matrix[matIdx][q].UnPack(&buf[bufIdx]);
        bufIdx += transactionSize;
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
// TODO use a tree structured broadcast. The processes are the nodes.
// starting from process 0 (which is assumed to be the root) each
// process will send only rows of the matrix that are needed by
// its children. This will reduce the communication by quite a bit.
void vtkMaterialInterfacePieceTransactionMatrix::Broadcast(vtkCommunicator* comm, int srcProc)
{
  int myProcId = comm->GetLocalProcessId();

  /*
  int nProcs=comm->GetNumberOfProcesses();

  // Identify the process rows which we need to send.
  // These will be sent in two sets. One set contains
  // all of the rows which the left child needs to send
  // to all its childeren. The other set contains those
  // for the right child.
  int leftChild=2*myProc+1;
  vector<int> &leftChildRows;
  ::IdentifyDownStreamRecipients(leftChild,nProcs,leftChildRows);
  sort(leftChildRows.begin(),leftChildRows.end());
  vector::size_type nLeftChildRows=leftChildRows.size();
  // TODO pack

  int rightChild=leftChild+1;
  vector<int> &rightChildRows;
  ::IdentifyDownStreamRecipients(rightChild,nProcs,rightChildRows);
  sort(rightChildRows.begin(),rightChildRows.end());
  vector::size_type nRightChildRows=rightChildRows.size();
  // TODO pack

  // source must send, and receives nothing.
  // everyone else receives from their parent
  // and if they have children sends to them.
  if (myProcId!=0)
    {
    int parent=(myProc-1)/2;
    // TODO receive from parent
    // TODO unpack
    }

  // TODO send to left child
  // TODO send to right child
  return;
  */

  // NOTE: this will be replaced by tree structured
  // broadcast.
  // pack
  int* buf = 0;
  int bufSize = 0;
  if (myProcId == srcProc)
  {
    bufSize = this->Pack(buf);
  }

  // move
  comm->Broadcast(&bufSize, 1, srcProc);
  if (myProcId != srcProc)
  {
    buf = new int[bufSize];
  }
  comm->Broadcast(buf, bufSize, srcProc);

  // unpack
  if (myProcId != srcProc)
  {
    this->UnPack(buf);
  }

  // clean up
  delete[] buf;
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfacePieceTransactionMatrix::Print()
{
  for (int j = 0; j < this->NProcs; ++j)
  {
    for (int i = 0; i < this->NFragments; ++i)
    {
      int matIdx = i + j * this->NFragments;
      int nTransactions = static_cast<int>(this->Matrix[matIdx].size());

      if (nTransactions > 0)
      {
        cerr << "TM[f=" << i << ",p=" << j << "]=";

        // put this i,j's transaction list
        for (int q = 0; q < nTransactions; ++q)
        {
          cerr << this->Matrix[matIdx][q] << ",";
        }
        cerr << endl;
      }
    }
  }
}
