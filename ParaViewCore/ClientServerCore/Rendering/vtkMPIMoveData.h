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
/**
 * @class   vtkMPIMoveData
 * @brief   Moves/redistributes data between processes.
 *
 * This class combines all the duplicate and collection requirements
 * into one filter. It can move polydata and unstructured grid between
 * processes. It can redistributed polydata from M to N processors.
 * Update: This filter can now support delivering vtkUniformGridAMR datasets in
 * PASS_THROUGH and/or COLLECT modes.
*/

#ifndef vtkMPIMoveData_h
#define vtkMPIMoveData_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPassInputTypeAlgorithm.h"

class vtkMultiProcessController;
class vtkSocketController;
class vtkMPIMToNSocketConnection;
class vtkDataSet;
class vtkIndent;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkMPIMoveData : public vtkPassInputTypeAlgorithm
{
public:
  static vtkMPIMoveData* New();
  vtkTypeMacro(vtkMPIMoveData, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Use this method to initialize all communicators/sockets using ParaView
   * defaults.
   */
  virtual void InitializeForCommunicationForParaView();

  //@{
  /**
   * Objects for communication.
   * The controller is an MPI controller used to communicate
   * between processes within one server (render or data).
   * The client-data server socket controller is set on the client
   * and data server and is used to communicate between the two.
   * MPIMToNSocetConnection is set on the data server and render server
   * when we are running with a render server.  It has multiple
   * sockets which are used to send data from the data server to the
   * render server.
   * ClientDataServerController==0  => One MPI program.
   * MPIMToNSocketConnection==0 => Client-DataServer.
   * MPIMToNSocketConnection==1 => Client-DataServer-RenderServer.
   */
  void SetController(vtkMultiProcessController* controller);
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection* sc);
  void SetClientDataServerSocketController(vtkMultiProcessController*);
  vtkGetObjectMacro(ClientDataServerSocketController, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Tell the object on which client/server it resides.
   * Whether the sockets are set helps determine which servers are running.
   */
  void SetServerToClient() { this->Server = vtkMPIMoveData::CLIENT; }
  void SetServerToDataServer() { this->Server = vtkMPIMoveData::DATA_SERVER; }
  void SetServerToRenderServer() { this->Server = vtkMPIMoveData::RENDER_SERVER; }
  vtkSetClampMacro(Server, int, vtkMPIMoveData::CLIENT, vtkMPIMoveData::RENDER_SERVER);
  vtkGetMacro(Server, int);
  //@}

  /**
   * Specify how the data is to be redistributed.
   */
  void SetMoveModeToPassThrough() { this->MoveMode = vtkMPIMoveData::PASS_THROUGH; }
  void SetMoveModeToCollect() { this->MoveMode = vtkMPIMoveData::COLLECT; }
  void SetMoveModeToClone() { this->MoveMode = vtkMPIMoveData::CLONE; }
  vtkSetClampMacro(
    MoveMode, int, vtkMPIMoveData::PASS_THROUGH, vtkMPIMoveData::COLLECT_AND_PASS_THROUGH);

  //@{
  /**
   * Controls the output type. This is required because processes receiving
   * data cannot know their output type in RequestDataObject without
   * communicating with other processes. Since communicating with other
   * processes in RequestDataObject is dangerous (can cause deadlock because
   * it may happen out-of-sync), the application has to set the output
   * type. The default is VTK_POLY_DATA.
   */
  vtkSetMacro(OutputDataType, int);
  vtkGetMacro(OutputDataType, int);
  //@}

  //@{
  /**
   * When set to true, zlib compression is used. False by default.
   * This value has any effect only on the data-sender processes. The receiver
   * always checks the received data to see if zlib decompression is required.
   */
  static void SetUseZLibCompression(bool b);
  static bool GetUseZLibCompression();
  //@}

  /**
   * vtkMPIMoveData doesn't necessarily generate a valid output data on all the
   * involved processes (depending on the MoveMode and Server ivars). This
   * returns true if valid data is available on the current processes after
   * successful Update() given the current ivars).
   */
  bool GetOutputGeneratedOnProcess();

  //@{
  /**
   * When set, vtkMPIMoveData will skip the gather-to-root-node process
   * altogether. This is useful when the data is already cloned on the
   * server-nodes or we are interested in the root-node result alone.
   */
  vtkSetMacro(SkipDataServerGatherToZero, bool);
  vtkGetMacro(SkipDataServerGatherToZero, bool);
  //@}

  enum MoveModes
  {
    PASS_THROUGH = 0,
    COLLECT = 1,
    CLONE = 2,
    COLLECT_AND_PASS_THROUGH = 3,
    INVALID
  };

protected:
  vtkMPIMoveData();
  ~vtkMPIMoveData() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkMultiProcessController* Controller;
  vtkMultiProcessController* ClientDataServerSocketController;
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;

  void DataServerAllToN(vtkDataObject* inData, vtkDataObject* outData, int n);
  void DataServerGatherAll(vtkDataObject* input, vtkDataObject* output);
  void DataServerGatherToZero(vtkDataObject* input, vtkDataObject* output);
  void DataServerSendToRenderServer(vtkDataObject* output);
  void RenderServerReceiveFromDataServer(vtkDataObject* output);
  void DataServerZeroSendToRenderServerZero(vtkDataObject* data);
  void RenderServerZeroReceiveFromDataServerZero(vtkDataObject* data);
  void RenderServerZeroBroadcast(vtkDataObject* data);
  void DataServerSendToClient(vtkDataObject* output);
  void ClientReceiveFromDataServer(vtkDataObject* output);

  int NumberOfBuffers;
  vtkIdType* BufferLengths;
  vtkIdType* BufferOffsets;
  char* Buffers;
  vtkIdType BufferTotalLength;

  void ClearBuffer();
  void MarshalDataToBuffer(vtkDataObject* data);
  void ReconstructDataFromBuffer(vtkDataObject* data);

  int MoveMode;
  int Server;

  bool SkipDataServerGatherToZero;

  enum Servers
  {
    CLIENT = 0,
    DATA_SERVER = 1,
    RENDER_SERVER = 2
  };

  int OutputDataType;

private:
  int UpdateNumberOfPieces;
  int UpdatePiece;

  vtkMPIMoveData(const vtkMPIMoveData&) = delete;
  void operator=(const vtkMPIMoveData&) = delete;

  static bool UseZLibCompression;
};

#endif
