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
/**
 * @class   vtkMaterialInterfacePieceTransactionMatrix
 *
 * Container to hold  a sets of transactions (sends/recvs)
 * indexed by fragment and proc, intended to facilitate
 * moving fragment pieces around.
 *
 * Internally we have a 2D matrix. On one axis is fragment id
 * on the other is proc id.
 *
 * Transaction are intended to execute in fragment order
 * so that no deadlocks occur.
*/

#ifndef vtkMaterialInterfacePieceTransactionMatrix_h
#define vtkMaterialInterfacePieceTransactionMatrix_h

#include "vtkMaterialInterfacePieceTransaction.h" //
#include "vtkPVVTKExtensionsDefaultModule.h"      //needed for exports
#include "vtkType.h"                              //
#include <vector>                                 //

class vtkCommunicator;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkMaterialInterfacePieceTransactionMatrix
{
public:
  /**
   * Set the object to an un-initialized state.
   */
  vtkMaterialInterfacePieceTransactionMatrix();
  /**
   * Allocate internal resources and set the object
   * to an initialized state.
   */
  vtkMaterialInterfacePieceTransactionMatrix(int nFragments, int nProcs);
  //@{
  /**
   * Free allocated resources and leave the object in an
   * un-initialized state.
   */
  ~vtkMaterialInterfacePieceTransactionMatrix();
  void Initialize(int nProcs, int nFragments);
  //@}
  /**
   * Free allocated resources and leave the object in an
   * un-initialized state.
   */
  void Clear();
  /**
   * Get the number of transaction a given process will
   * execute.
   */
  vtkIdType GetNumberOfTransactions(int procId);
  /**
   * Given a proc and a fragment, return a ref to
   * the associated list of transactions.
   */
  std::vector<vtkMaterialInterfacePieceTransaction>& GetTransactions(int fragmentId, int procId);
  /**
   * Add a transaction to the end of the given a proc,fragment pair's
   * transaction list.
   */
  void PushTransaction(
    int fragmentId, int procId, vtkMaterialInterfacePieceTransaction& transaction);
  //@{
  /**
   * Send the transaction matrix on srcProc to all
   * other procs.
   */
  void Broadcast(vtkCommunicator* comm, int srcProc);
  //
  void Print();
  //@}
  /**
   * Tells how much memory the matrix has allocated.
   */
  vtkIdType Capacity()
  {
    return this->FlatMatrixSize +
      this->NumberOfTransactions * sizeof(vtkMaterialInterfacePieceTransaction);
  }

private:
  /**
   * Put the matrix into a buffer for communication.
   * returns size of the buffer in ints. The buffer
   * will be allocated, and is expected to be null
   * on entry.
   */
  vtkIdType Pack(int*& buffer);
  /**
   * Put a set of rows into a buffer for communication.
   * This is used to send a subset of the TM.
   */
  vtkIdType PackPartial(int*& buffer, int* rows, int nRows);
  //@{
  /**
   * Load state from a buffer containing a Pack'ed
   * transaction matrix. 0 is returned on error.
   */
  int UnPack(int* buffer);
  int UnPackPartial(int* buffer);
  ///
  int NProcs;
  int NFragments;
  std::vector<vtkMaterialInterfacePieceTransaction>* Matrix;
  vtkIdType FlatMatrixSize;
  vtkIdType NumberOfTransactions;
};
//
inline std::vector<vtkMaterialInterfacePieceTransaction>&
vtkMaterialInterfacePieceTransactionMatrix::GetTransactions(int fragmentId, int procId)
{
  int idx = fragmentId + procId * this->NFragments;
  return this->Matrix[idx];
}
//
inline vtkIdType vtkMaterialInterfacePieceTransactionMatrix::GetNumberOfTransactions(int procId)
{
  size_t nTransactions = 0;
  //@}

  for (int fragmentId = 0; fragmentId < this->NFragments; ++fragmentId)
  {
    nTransactions += this->GetTransactions(fragmentId, procId).size();
  }

  return static_cast<vtkIdType>(nTransactions);
}
#endif

// VTK-HeaderTest-Exclude: vtkMaterialInterfacePieceTransactionMatrix.h
