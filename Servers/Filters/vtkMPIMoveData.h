/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIMoveData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMPIMoveData - For distributed tiled displays.
//
// .SECTION Description
// This class combines all the duplicate and collection requirements
// into one filter.

#ifndef __vtkMPIMoveData_h
#define __vtkMPIMoveData_h

#include "vtkDataSetToDataSetFilter.h"
class vtkMultiProcessController;
class vtkSocketController;
class vtkMPIMToNSocketConnection;
class vtkDataSet;
class vtkIndent;

class VTK_EXPORT vtkMPIMoveData : public vtkDataSetToDataSetFilter
{
public:
  static vtkMPIMoveData *New();
  vtkTypeRevisionMacro(vtkMPIMoveData, vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods assume the user knows the output type,
  // a creates the output if necessary even when the input has
  // not been set yet.
  virtual vtkPolyData* GetPolyDataOutput();
  virtual vtkUnstructuredGrid* GetUnstructuredGridOutput();
  virtual vtkDataSet* GetOutput();

  // Description:
  // Objects for communication.
  // The controller is an MPI controller used to communicate
  // between processes within one server (render or data).
  // The client-data server socket controller is set on the client
  // and data server and is used to communicate between the two.
  // MPIMToNSocetConnection is set on the data server and render server 
  // when we are running with a render server.  It has multiple
  // sockets which are used to send data from the data server to the 
  // render server.
  // ClientDataServerController==0  => One MPI program.
  // MPIMToNSocketConnection==0 => Client-DataServer.
  // MPIMToNSocketConnection==1 => Client-DataServer-RenderServer.
  void SetController(vtkMultiProcessController* controller);
  void SetClientDataServerSocketController(vtkSocketController* sdc);
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection* sc);
  
  // Description:
  // Tell the object on which client/server it resides.
  // Whether the sockets are set helps determine which servers are running.
  void SetServerToClient(){this->Server=vtkMPIMoveData::CLIENT;}
  void SetServerToDataServer(){this->Server=vtkMPIMoveData::DATA_SERVER;}
  void SetServerToRenderServer(){this->Server=vtkMPIMoveData::RENDER_SERVER;}

  // Description:
  // Specify how the data is to be redistributed.
  void SetMoveModeToPassThrough(){this->MoveMode=vtkMPIMoveData::PASS_THROUGH;}
  void SetMoveModeToCollect(){this->MoveMode=vtkMPIMoveData::COLLECT;}
  void SetMoveModeToClone(){this->MoveMode=vtkMPIMoveData::CLONE;}

  // Description:
  // The old classes cloned when mode was collect.  It is easier (and cleaner)
  // to support this than change the part display superclasses method
  // "SetCollectionDecision".
  vtkSetMacro(DefineCollectAsClone,int);
  vtkGetMacro(DefineCollectAsClone,int);
  vtkBooleanMacro(DefineCollectAsClone,int);

  // Description:
  // Legacy API for ParaView 1.4
  void SetPassThrough(int v) 
    {if(v){this->SetMoveModeToPassThrough();} else {this->SetMoveModeToClone();}}
  void SetSocketController(vtkSocketController* c) {this->SetClientDataServerSocketController(c);}


protected:
  vtkMPIMoveData();
  ~vtkMPIMoveData();

  vtkMultiProcessController* Controller;
  vtkSocketController* ClientDataServerSocketController;
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;

  // Data generation method
  virtual void ComputeInputUpdateExtents(vtkDataObject *output);
  virtual void Execute();
  virtual void ExecuteInformation();

  void DataServerAllToN(vtkDataSet* inData, vtkDataSet* outData, int n);
  void DataServerGatherAll(vtkDataSet* input, vtkDataSet* output);
  void DataServerGatherToZero(vtkDataSet* input, vtkDataSet* output);
  void DataServerSendToRenderServer(vtkDataSet* output);
  void RenderServerReceiveFromDataServer(vtkDataSet* output);
  void DataServerZeroSendToRenderServerZero(vtkDataSet* data);
  void RenderServerZeroReceiveFromDataServerZero(vtkDataSet* data);
  void RenderServerZeroBroadcast(vtkDataSet* data);
  void DataServerSendToClient(vtkDataSet* output);
  void ClientReceiveFromDataServer(vtkDataSet* output);

  int   NumberOfBuffers;
  int*  BufferLengths;
  int*  BufferOffsets;
  char* Buffers;
  int   BufferTotalLength;

  void ClearBuffer();
  void MarshalDataToBuffer(vtkDataSet* data);
  void ReconstructDataFromBuffer(vtkDataSet* data);

  int MoveMode;
  int Server;

  int DefineCollectAsClone;
//BTX
  enum MoveModes {
    PASS_THROUGH=0,
    COLLECT=1,
    CLONE=2
  };
//ETX

//BTX
  enum Servers {
    CLIENT=0,
    DATA_SERVER=1,
    RENDER_SERVER=2
  };
//ETX

private:
  vtkMPIMoveData(const vtkMPIMoveData&); // Not implemented
  void operator=(const vtkMPIMoveData&); // Not implemented
};

#endif

