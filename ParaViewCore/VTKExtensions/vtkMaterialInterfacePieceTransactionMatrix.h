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
// .NAME vtkMaterialInterfacePieceTransactionMatrix
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

#ifndef __vtkMaterialInterfacePieceTransactionMatrix_h
#define __vtkMaterialInterfacePieceTransactionMatrix_h

#include "vtkMaterialInterfacePieceTransaction.h" //
#include "vtkType.h" //
#include "vector" //

class vtkCommunicator;

class vtkMaterialInterfacePieceTransactionMatrix
{
public:
  // Description:
  // Set the object to an un-initialized state.
  vtkMaterialInterfacePieceTransactionMatrix();
  // Description:
  // Allocate internal resources and set the object
  // to an initialized state.
  vtkMaterialInterfacePieceTransactionMatrix(int nFragments, int nProcs);
  // Description:
  // Free allocated resources and leave the object in an
  // un-initialized state.
  ~vtkMaterialInterfacePieceTransactionMatrix();
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
  std::vector<vtkMaterialInterfacePieceTransaction> &GetTransactions(
                  int fragmentId,
                  int procId);
  // Description:
  // Add a transaction to the end of the given a proc,fragment pair's
  // transaction list.
  void PushTransaction(
                  int fragmentId,
                  int procId,
                  vtkMaterialInterfacePieceTransaction &transaction);
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
      + this->NumberOfTransactions*sizeof(vtkMaterialInterfacePieceTransaction);
  }
private:
  // Description:
  // Put the matrix into a buffer for communication.
  // returns size of the buffer in ints. The buffer
  // will be allocated, and is expected to be null
  // on entry.
  vtkIdType Pack(int *&buffer);
  // Description:
  // Put a set of rows into a buffer for communication.
  // This is used to send a subset of the TM.
  vtkIdType PackPartial(int *&buffer, int *rows, int nRows);
  // Description:
  // Load state from a buffer containing a Pack'ed
  // transaction matrix. 0 is returned on error.
  int UnPack(int *buffer);
  int UnPackPartial(int *buffer);
  ///
  int NProcs;
  int NFragments;
  std::vector<vtkMaterialInterfacePieceTransaction> *Matrix;
  vtkIdType FlatMatrixSize;
  vtkIdType NumberOfTransactions;
};
//
inline
std::vector<vtkMaterialInterfacePieceTransaction> &
vtkMaterialInterfacePieceTransactionMatrix::GetTransactions(
                int fragmentId,
                int procId)
{
  int idx=fragmentId+procId*this->NFragments;
  return this->Matrix[idx];
}
//
inline
vtkIdType
vtkMaterialInterfacePieceTransactionMatrix::GetNumberOfTransactions(
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
