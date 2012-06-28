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
// .NAME vtkMaterialInterfacePieceTransaction
// .SECTION Description
// Helper to the transaction matrix.
//
// Data structure that describes a single transaction
// that needs to be executed in the process of
// moving a fragment piece around.
//
// The fragment to be transacted and the executing process are
// determined implicitly by where the transaction is stored.

#ifndef __vtkMaterialInterfacePieceTransaction_h
#define __vtkMaterialInterfacePieceTransaction_h

#include "vtkSystemIncludes.h"

#include "vtksys/ios/iostream"

class vtkMaterialInterfacePieceTransaction
{
public:
  enum {TYPE=0,REMOTE_PROC=1,SIZE=2};
  //
  vtkMaterialInterfacePieceTransaction(){ Clear(); }
  ~vtkMaterialInterfacePieceTransaction(){ Clear(); }
  //
  vtkMaterialInterfacePieceTransaction(
                  char type,
                  int remoteProc)
  {
    this->Initialize(type,remoteProc);
  }
  //
  void Initialize(char type,
                  int remoteProc)
  {
    this->Data[TYPE]=(int)type;
    this->Data[REMOTE_PROC]=remoteProc;
  }
  //
  bool Empty() const{ return this->Data[TYPE]==0; }
  //
  void Clear()
  {
    this->Data[TYPE]=0;
    this->Data[REMOTE_PROC]=-1;
  }
  //
  void Pack(int *buf)
  {
    buf[0]=this->Data[TYPE];
    buf[1]=this->Data[REMOTE_PROC];
  }
  //
  void UnPack(int *buf)
  {
    this->Data[TYPE]=buf[0];
    this->Data[REMOTE_PROC]=buf[1];
  }
  //
  char GetType() const{ return (char)this->Data[TYPE]; }
  //
  int GetRemoteProc() const{ return this->Data[REMOTE_PROC]; }
  //
  int GetFlatSize() const{ return SIZE; }
private:
  int Data[SIZE];
};
vtksys_ios::ostream &operator<<(vtksys_ios::ostream &sout, const vtkMaterialInterfacePieceTransaction &ta);
#endif
