/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentPieceTransactionMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentPieceTransactionMatrix
// .SECTION Description
// Container to hold  a sets of transactions (sends/recvs)
// indexed by fragment and proc, inteneded to facilitiate
// moving fragment peices around.
//
// Internaly we have a 2D matrix. On one axis is fragment id
// on the other is proc id.
//
// Transaction are intended to execute in fragment order
// so that no deadlocks occur.

#ifndef __vtkCTHFragmentPieceTransactionMatrix_h
#define __vtkCTHFragmentPieceTransactionMatrix_h

#include "vtkCTHFragmentPieceTransaction.h"
#include "vtkType.h"
#include "vtkstd/vector"

class vtkCommunicator;


class vtkCTHFragmentPieceTransactionMatrix
{
  public:
    // Description:
    // Set the object to an un-initialized state.
    vtkCTHFragmentPieceTransactionMatrix();
    // Description:
    // Allocate internal resources and set the object
    // to an initialized state.
    vtkCTHFragmentPieceTransactionMatrix(int nFragments, int nProcs);
    // Description:
    // Free allocated resources and leave the object in an
    // un-initialized state.
    ~vtkCTHFragmentPieceTransactionMatrix();
    void Initialize(int nProcs, int nFragments);
    // Description:
    // Free allocated resources and leave the object in an
    // un-initialized state.
    void Clear();
    // Description:
    // Get the number of transaction a given process will
    // execute.
    vtkIdType GetNumberOfTransactions(int procId);
    // Description:
    // Given a proc and a fragment, return a ref to
    // the associated list of tranactions.
    vtkstd::vector<vtkCTHFragmentPieceTransaction> &GetTransactions(
                    int fragmentId,
                    int procId);
    // Description:
    // Add a transaction to the end of the given a proc,fragment pair's
    // transaction list.
    void PushTransaction(
                    int fragmentId,
                    int procId,
                    vtkCTHFragmentPieceTransaction &transaction);
    // Description:
    // Send the transaction matrix on srcProc to all
    // other procs.
    void Broadcast(vtkCommunicator *comm, int srcProc);
    //
    void Print();
    // Description:
    // Tells how much memory the matrix has allocated.
    vtkIdType Capacity()
    {
      return this->FlatMatrixSize
        + this->NumberOfTransactions*sizeof(vtkCTHFragmentPieceTransaction);
    }
  private:
    // Put the matrix into a buffer for communication.
    // returns size of the buffer in ints. The buffer
    // will be allocated, and is expected to be null
    // on entry.
    vtkIdType Pack(int *&buffer);
    vtkIdType PackRow(int *&buffer);
    // Load state from a buffer containing a Pack'ed
    // transaction matrix. 0 is returned on error.
    int UnPack(int *buffer);
    int UnPackRow(int *buffer);
    int NProcs;
    int NFragments;
    vtkstd::vector<vtkCTHFragmentPieceTransaction> *Matrix;
    vtkIdType FlatMatrixSize;
    vtkIdType NumberOfTransactions;
};
//
inline
vtkstd::vector<vtkCTHFragmentPieceTransaction> &
vtkCTHFragmentPieceTransactionMatrix::GetTransactions(
                int fragmentId,
                int procId)
{
  int idx=fragmentId+procId*this->NFragments;
  return this->Matrix[idx];
}
//
inline 
vtkIdType 
vtkCTHFragmentPieceTransactionMatrix::GetNumberOfTransactions(
                int procId)
{
  int nTransactions=0;

  for (int fragmentId=0; fragmentId<this->NFragments; ++fragmentId)
    {
    nTransactions+=this->GetTransactions(fragmentId,procId).size();
    }

  return nTransactions;
}
#endif
