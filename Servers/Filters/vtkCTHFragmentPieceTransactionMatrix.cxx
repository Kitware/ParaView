/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentPieceTransactionMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFragmentPieceTransactionMatrix.h"
#include "vtkCommunicator.h"
#include "vtksys/ios/iostream"
#include <cassert>
using vtkstd::vector;

//
vtkCTHFragmentPieceTransactionMatrix::vtkCTHFragmentPieceTransactionMatrix()
{
  this->NFragments=0;
  this->NProcs=0;
  this->FlatMatrixSize=0;
  this->Matrix=0;
  this->NumberOfTransactions=0;
}
//
vtkCTHFragmentPieceTransactionMatrix::vtkCTHFragmentPieceTransactionMatrix(
                int nFragments,
                int nProcs)
{
  this->Initialize(nFragments, nProcs);
}
//
vtkCTHFragmentPieceTransactionMatrix::~vtkCTHFragmentPieceTransactionMatrix()
{
  this->Clear();
}
//
void vtkCTHFragmentPieceTransactionMatrix::Clear()
{
  this->NFragments=0;
  this->NProcs=0;
  if ( this->Matrix )
    {
    delete [] this->Matrix;
    this->Matrix=0;
    }
  this->NumberOfTransactions=0;
}
//
void vtkCTHFragmentPieceTransactionMatrix::Initialize(
                int nFragments,
                int nProcs)
{
  this->Clear();

  this->NFragments=nFragments;
  this->NProcs=nProcs;
  this->FlatMatrixSize=nFragments*nProcs;
  this->Matrix 
    = new vector<vtkCTHFragmentPieceTransaction>[this->FlatMatrixSize];
}
//
void vtkCTHFragmentPieceTransactionMatrix::PushTransaction(
                int fragmentId,
                int procId,
                vtkCTHFragmentPieceTransaction &transaction)
{
  int idx=fragmentId+procId*this->NFragments;
  this->Matrix[idx].push_back(transaction);
  ++this->NumberOfTransactions;
}
// 
vtkIdType vtkCTHFragmentPieceTransactionMatrix::PackRow(int *&buf)
{
/*
the packed row has this structure:

[nT,T0,...TN][nT,T0,...,TN]...[nT,T0,...,TN]
    /\           /\               /\
    |            |                |
  f0,p0        f1,p0            fN,p0 
*/

  // caller should not allocate memory.
  assert( "Buffer appears to be pre-allocated."
          && buf==0 );

  //int matIdx=i+j*this->NFragments;
  //int nTransactions=this->Matrix[matIdx].size();

  const int transactionSize
    = vtkCTHFragmentPieceTransaction::SIZE;

  const vtkIdType bufSize = this->NFragments         // transaction count for each fragment
        + transactionSize*this->NumberOfTransactions // enough to store all transactions
        + 2;                                         // nFragments, nProcs

  buf = new int[bufSize];
  // header
  buf[0]=this->NFragments;
  buf[1]=this->NProcs;
  vtkIdType bufIdx=2;

  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      int matIdx=i+j*this->NFragments;
      int nTransactions=this->Matrix[matIdx].size();

      // put the count for this i,j
      buf[bufIdx]=nTransactions;
      ++bufIdx;

      // put this i,j's transaction list
      for (int q=0; q<nTransactions; ++q)
        {
        this->Matrix[matIdx][q].Pack(&buf[bufIdx]);
        bufIdx+=transactionSize;
        }
      }
    }
  // now bufIdx is number of ints we have stored
  return bufIdx;
}
// 
vtkIdType vtkCTHFragmentPieceTransactionMatrix::Pack(int *&buf)
{
/*
the buffer has this structure:

[nFragments,nProcs][nT,T0,...TN][nT,T0,...,TN]...[nT,T0,...,TN]...[nT,T0,...,TN]
                       /\           /\               /\               /\
                       |            |                |                |
                      f0,p0        f1,p0            fN,p0            fN,PN
*/

  // caller should not allocate memory.
  assert( "Buffer appears to be pre-allocated."
          && buf==0 );

  const int transactionSize
    = vtkCTHFragmentPieceTransaction::SIZE;

  const vtkIdType bufSize = this->FlatMatrixSize     // transaction count for each i,j
        + transactionSize*this->NumberOfTransactions // enough to store all transactions
        + 2;                                         // nFragments, nProcs

  buf = new int[bufSize];
  // header
  buf[0]=this->NFragments;
  buf[1]=this->NProcs;
  vtkIdType bufIdx=2;

  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      int matIdx=i+j*this->NFragments;
      int nTransactions=this->Matrix[matIdx].size();

      // put the count for this i,j
      buf[bufIdx]=nTransactions;
      ++bufIdx;

      // put this i,j's transaction list
      for (int q=0; q<nTransactions; ++q)
        {
        this->Matrix[matIdx][q].Pack(&buf[bufIdx]);
        bufIdx+=transactionSize;
        }
      }
    }
  // now bufIdx is number of ints we have stored
  return bufIdx;
}
//
int vtkCTHFragmentPieceTransactionMatrix::UnPack(int *buf)
{
  assert("Buffer has not been allocated."
         && buf!=0 );

  const int transactionSize
    = vtkCTHFragmentPieceTransaction::SIZE;

  this->Initialize(buf[0],buf[1]);
  int bufIdx=2;

  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      // get the number of transactions for this i,j
      int nTransactions=buf[bufIdx];
      ++bufIdx;

      // size the i,j th transaction list
      int matIdx=i+j*this->NFragments;
      this->Matrix[matIdx].resize(nTransactions);

      // load the i,j th transaction list
      for (int q=0; q<nTransactions; ++q)
        {
        this->Matrix[matIdx][q].UnPack(&buf[bufIdx]);
        bufIdx+=transactionSize;
        }
      }
    }

  return 1;
}
//
void vtkCTHFragmentPieceTransactionMatrix::Broadcast(
                vtkCommunicator *comm,
                int srcProc)
{
  int myProc=comm->GetLocalProcessId();

  // pack
  int *buf=0;
  int bufSize=0;
  if ( myProc==srcProc )
    {
    bufSize=this->Pack(buf);
    }

  // move
  comm->Broadcast(&bufSize,1,srcProc);
  if ( myProc!=srcProc )
    {
    buf = new int[bufSize];
    }
  comm->Broadcast(buf,bufSize,srcProc);

  // unpack
  if ( myProc!=srcProc )
    {
    this->UnPack(buf);
    }

  // clean up
  delete [] buf;
}
//
void vtkCTHFragmentPieceTransactionMatrix::Print()
{
  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      int matIdx=i+j*this->NFragments;
      int nTransactions=this->Matrix[matIdx].size();

      if (nTransactions>0)
        {
        cerr << "TM[f=" << i << ",p=" << j << "]=";

        // put this i,j's transaction list
        for (int q=0; q<nTransactions; ++q)
          {
          cerr << this->Matrix[matIdx][q] << ",";
          }
        cerr << endl;
        }
      }
    }
}
