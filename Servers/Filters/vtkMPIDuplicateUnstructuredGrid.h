/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIDuplicateUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMPIDuplicateUnstructuredGrid - For distributed tiled displays.
//
// .SECTION Description
// This filter collects polydata and duplicates it on every node.
// Converts data parallel so every node has a complete copy of the data.
// The filter is used at the end of a pipeline for driving a tiled
// display. This version uses MPI Gather and Broadcast.


#ifndef __vtkMPIDuplicateUnstructuredGrid_h
#define __vtkMPIDuplicateUnstructuredGrid_h

#include "vtkUnstructuredGridToUnstructuredGridFilter.h"
class vtkSocketController;
class vtkMultiProcessController;
class vtkUnstructuredGridWriter;
class vtkUnstructuredGridReader;


class VTK_EXPORT vtkMPIDuplicateUnstructuredGrid : public vtkUnstructuredGridToUnstructuredGridFilter
{
public:
  static vtkMPIDuplicateUnstructuredGrid *New();
  vtkTypeRevisionMacro(vtkMPIDuplicateUnstructuredGrid, vtkUnstructuredGridToUnstructuredGridFilter);
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
  // Both processes think their id is 0.
  vtkSocketController *GetSocketController() {return this->SocketController;}
  void SetSocketController (vtkSocketController *controller);
  vtkSetMacro(ClientFlag,int);
  vtkGetMacro(ClientFlag,int);

  // Description:
  // These methods extend this filter to work with a render server.
  // If the RenderServerSocket is set, then the fitlers assumes it 
  // is operating in render server mode. 
  // The RenderServerFlag differentiates between data and render servers.
  // The client does not care.
  vtkSocketController *GetRenderServerSocketController() 
    {return this->RenderServerSocketController;}
  void SetRenderServerSocketController (vtkSocketController *controller);
  vtkSetMacro(RenderServerFlag,int);
  vtkGetMacro(RenderServerFlag,int);

  // Description:
  // Turn the filter on or off.  ParaView disable this filter when it will
  // use compositing instead of local rendering.  This flag is off by default.
  vtkSetMacro(PassThrough,int);
  vtkGetMacro(PassThrough,int);
  vtkBooleanMacro(PassThrough,int);

  // Description:
  // This flag should be set on all processes when MPI root
  // is used as client.
  vtkSetMacro(ZeroEmpty,int);
  vtkGetMacro(ZeroEmpty,int);
  vtkBooleanMacro(ZeroEmpty,int);

  // Description:
  // This returns to size of the output (on this process).
  // This method is not really used.  It is needed to have
  // the same API as vtkCollectUnstructuredGrid.
  //vtkGetMacro(MemorySize, unsigned long);
  
  
protected:
  vtkMPIDuplicateUnstructuredGrid();
  ~vtkMPIDuplicateUnstructuredGrid();

  // Data generation method
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ServerExecute(vtkUnstructuredGridReader* reader, 
                     vtkUnstructuredGridWriter* writer);
  void RenderServerExecute(vtkUnstructuredGridReader* reader);
  void ClientExecute(vtkUnstructuredGridReader* reader);
  void ReconstructOutput(vtkUnstructuredGridReader* reader, int numProcs,
                         char* recv, int* recvLengths, int* recvOffsets);
  void ExecuteInformation();

  vtkMultiProcessController *Controller;

  // For client server mode.
  vtkSocketController *SocketController;
  int ClientFlag;

  // For render server mode.
  vtkSocketController *RenderServerSocketController;
  int RenderServerFlag;

  //unsigned long MemorySize;
  int PassThrough;
  int ZeroEmpty;


private:
  vtkMPIDuplicateUnstructuredGrid(const vtkMPIDuplicateUnstructuredGrid&); // Not implemented
  void operator=(const vtkMPIDuplicateUnstructuredGrid&); // Not implemented
};

#endif

