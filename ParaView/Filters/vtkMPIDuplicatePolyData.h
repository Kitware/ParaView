/*=========================================================================

  Program:   ParaView
  Module:    vtkMPIDuplicatePolyData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPIDuplicatePolyData - For distributed tiled displays.
// .DESCRIPTION
// This filter collects poly data and duplicates it on every node.
// Converts data parallel so every node has a complete copy of the data.
// The filter is used at the end of a pipeline for driving a tiled
// display.  This version uses MPI Gather and Broadcast.


#ifndef __vtkMPIDuplicatePolyData_h
#define __vtkMPIDuplicatePolyData_h

#include "vtkPolyDataToPolyDataFilter.h"
class vtkSocketController;
class vtkMultiProcessController;
class vtkMPICommunicator;
class vtkPolyDataWriter;
class vtkPolyDataReader;


class VTK_EXPORT vtkMPIDuplicatePolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkMPIDuplicatePolyData *New();
  vtkTypeRevisionMacro(vtkMPIDuplicatePolyData, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // This duplicate filter works in client server mode when this
  // controller is set.  We have a client flag to diferentiate the
  // client and server because the socket controller is odd:
  // Proth processes think their id is 0.
  vtkSocketController *GetSocketController() {return this->SocketController;}
  void SetSocketController (vtkSocketController *controller);
  vtkSetMacro(ClientFlag,int);
  vtkGetMacro(ClientFlag,int);

  // Description:
  // This returns to size of the output (on this process).
  // This method is not really used.  It is needed to have
  // the same API as vtkCollectPolyData.
  vtkGetMacro(MemorySize, unsigned long);

protected:
  vtkMPIDuplicatePolyData();
  ~vtkMPIDuplicatePolyData();

  // Data generation method
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void RootExecute(vtkMPICommunicator* com, vtkPolyDataReader* reader, 
                   vtkPolyDataWriter* writer);
  void SateliteExecute(vtkMPICommunicator* com, vtkPolyDataReader* reader, 
                       vtkPolyDataWriter* writer);
  void ClientExecute(vtkPolyDataReader* reader);
  void ExecuteInformation();

  vtkMultiProcessController *Controller;

  // For client server mode.
  vtkSocketController *SocketController;
  int ClientFlag;

  unsigned long MemorySize;

private:
  vtkMPIDuplicatePolyData(const vtkMPIDuplicatePolyData&); // Not implemented
  void operator=(const vtkMPIDuplicatePolyData&); // Not implemented
};

#endif

