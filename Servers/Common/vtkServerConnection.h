/*=========================================================================

  Program:   ParaView
  Module:    vtkServerConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkServerConnection -- represents a connection with a server 
// (data/render).
// .SECTION Description
// This is a remote connection with a \b Server. This class is only instantiated 
// on the Client. It encapsulates the channel to the server.
// It maintains separate channels for RenderServer and DataServer, if needed.

#ifndef __vtkServerConnection_h
#define __vtkServerConnection_h

#include "vtkRemoteConnection.h"

class vtkPVServerInformation;

class VTK_EXPORT vtkServerConnection : public vtkRemoteConnection
{
public:
  static vtkServerConnection* New();
  vtkTypeMacro(vtkServerConnection, vtkRemoteConnection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes the connection. This is essential to
  // intialize the controller associated with the connection etc etc.
  // This sets up the RMIs and returns. 
  virtual int Initialize(int argc, char** argv, int *partitionId); 
 
  // Description:
  // Finalizes the connection.
  virtual void Finalize();
  
//BTX
  // Description:
  // Obtain the last result from the appropriate server.
  // Based on the serverFlags, this will request the information
  // from either the data or the render server.
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 serverFlags);
  
  // Description:
  // Gather the information about the object from the server.
  virtual void GatherInformation(vtkTypeUInt32 serverFlags, vtkPVInformation* info, 
    vtkClientServerID id);

  // Description:
  vtkGetMacro(MPIMToNSocketConnectionID, vtkClientServerID);
//ETX

  // Description:
  // Set the "live" socket to use for this connection. This must be
  // set before Initialize() is called. The SocketController will 
  // use this socket for all the communication.
  virtual int SetRenderServerSocket(vtkClientSocket* soc);

  // Description:
  // Server information was initially developed to query the
  // server whether it supports remote rendering.
  vtkGetObjectMacro(ServerInformation, vtkPVServerInformation);

  // Get the socket controller used for RenderServer, if any.
  vtkSocketController* GetRenderServerSocketController() 
    { return this->RenderServerSocketController; }

  // Description:
  // Get the number of data server processes on the server.
  virtual int GetNumberOfPartitions() { return this->NumberOfServerProcesses; }

  // Description:
  // Push the vtkUndoSet xml state on the undo stack for this connection.
  // Subclasses override this method to do the appropriate action.
  // On SelfConnection, the undo set is stored locally, while on
  // remote server connection, the undo set is sent to the server.
  virtual void PushUndo(const char* label, vtkPVXMLElement* root);

  // Description:
  // Get the next undo  xml from this connection.
  // This method allocates  a new vtkPVXMLElement. It is the responsibility 
  // of caller to \c Delete it. 
  // \returns NULL on failure, otherwise the XML element is returned.
  virtual vtkPVXMLElement* NewNextUndo();
 
  // Description:
  // Get the next redo  xml from this connection.
  // This method allocates  a new vtkPVXMLElement. It is the responsibility 
  // of caller to \c Delete it. 
  // \returns NULL on failure, otherwise the XML element is returned.
  virtual vtkPVXMLElement* NewNextRedo();

protected:
  vtkServerConnection();
  ~vtkServerConnection();
 
  // Description:
  // Creates flags from serverFlags which indicate where the stream is 
  // to be processed.
  virtual vtkTypeUInt32 CreateSendFlag(vtkTypeUInt32 serverFlags);
  // send a stream to the data server
  virtual int SendStreamToDataServer(vtkClientServerStream&);
  // send a stream to the data server root mpi process
  virtual int SendStreamToDataServerRoot(vtkClientServerStream&);
  // send a stream to the render server
  virtual int SendStreamToRenderServer(vtkClientServerStream&);
  // send a stream to the render server root mpi process
  virtual int SendStreamToRenderServerRoot(vtkClientServerStream&);
  // send a stream to the client.
  virtual int SendStreamToClient(vtkClientServerStream&);

  // Description:
  // Helper methods.
  int SendStreamToServer(vtkSocketController* controller,
    vtkClientServerStream& stream);
  int SendStreamToRoot(vtkSocketController* controller,
  vtkClientServerStream& stream);

  // Description:
  // Authenticates with the Server. Returns 1 on success, 0 on failure.
  int AuthenticateWithServer(vtkSocketController*);

  // Description:
  // Sets up communication between DataServer and RenderServer.
  int SetupDataServerRenderServerConnection();
  
  // Description:
  // Internal method that gather information from appropriate controller.
  void GatherInformationFromController(vtkSocketController* controller, 
    vtkPVInformation* info, vtkClientServerID id);
  
  // Description:
  // Internal method to obtain the last result.
  const vtkClientServerStream& GetLastResultInternal(
    vtkSocketController* controller);

  // Description:
  // Overridden to flag an error on the client.
  virtual void OnSocketError();
  
  // RenderServerSocketController is not created by default. It will only be 
  // instantiated when SetRenderServerSocket() is called.
  vtkSocketController* RenderServerSocketController;

  // The documentation says that this is used to choose filters. So we keep 
  // it for now.
  int NumberOfServerProcesses;

  // ID of the connector between RenderServer/DataServer, if any.
  vtkClientServerID MPIMToNSocketConnectionID;

  vtkPVServerInformation* ServerInformation;
  vtkClientServerStream* LastResultStream;
private:
  vtkServerConnection(const vtkServerConnection&); // Not implemented.
  void operator=(const vtkServerConnection&); // Not implemented.

};


#endif

