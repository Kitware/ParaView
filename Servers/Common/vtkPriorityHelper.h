/*=========================================================================

  Program:   ParaView
  Module:    vtkPriorityHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPriorityHelper - updates input conditionally
// .SECTION Description
// During info gathering, Streaming paraview adds this to the end of the 
// pipeline so that it can check each piece's priority before updating the 
// piece. If a piece has a priority of 0, it is skipped and the rest tested 
// in sequence.

#ifndef __vtkPriorityHelper_h
#define __vtkPriorityHelper_h

#include "vtkObject.h"

class vtkAlgorithmOutput;
class vtkAlgorithm;
class vtkDataObject;

class VTK_EXPORT vtkPriorityHelper : public vtkObject
{
public:
  static vtkPriorityHelper* New();
  vtkTypeMacro(vtkPriorityHelper, vtkObject);
  
  void SetInputConnection(vtkAlgorithmOutput *input);

  int SetSplitUpdateExtent(int port, 
                           int piece, int offset,
                           int numPieces, 
                           int nPasses,
                           int ghostLevel,
                           int save);
  
  virtual double ComputePriority();

  vtkDataObject *ConditionallyGetDataObject();
  void ConditionallyUpdate();

  void EnableStreamMessagesOn() { this->EnableStreamMessages = 1;}
  void EnableCullingOff() { this->EnableCulling = 0;}
protected:
  vtkPriorityHelper();
  ~vtkPriorityHelper();

  vtkDataObject *InternalUpdate(bool ReturnObject);

  vtkAlgorithm *Input;
  int Port;
  int Piece;
  int Offset;
  int NumPieces;
  int NumPasses;

  int EnableCulling;
  int EnableStreamMessages;
private:
  vtkPriorityHelper(const vtkPriorityHelper&); // Not implemented
  void operator=(const vtkPriorityHelper&); // Not implemented
};

#endif
