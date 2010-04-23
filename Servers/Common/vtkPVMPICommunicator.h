/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPICommunicator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMPICommunicator - subclass to read progress events as and when
// they appear.
// .SECTION Description
// vtkPVMPICommunicator overrides the ReceiveVoidArray() to use a asynchoronous
// receive to read in progress events sent by statellites as they appear.

#ifndef __vtkPVMPICommunicator_h
#define __vtkPVMPICommunicator_h

#include "vtkMPICommunicator.h"

class VTK_EXPORT vtkPVMPICommunicator : public vtkMPICommunicator
{
public:
  static vtkPVMPICommunicator* New();
  vtkTypeMacro(vtkPVMPICommunicator, vtkMPICommunicator);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVMPICommunicator();
  ~vtkPVMPICommunicator();

  // Description:
  // Implementation for receive data.
  // Overridden to do a polling receive to read progress events as well.
  virtual int ReceiveDataInternal(
    char* data, int length, int sizeoftype, 
    int remoteProcessId, int tag,
    vtkMPICommunicatorReceiveDataInfo* info,
    int useCopy, int& senderId);

private:
  vtkPVMPICommunicator(const vtkPVMPICommunicator&); // Not implemented
  void operator=(const vtkPVMPICommunicator&); // Not implemented
//ETX
};

#endif

