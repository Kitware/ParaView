/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessModuleConnection - abstract base class for
// client-server connections.
// .SECTION Description
// This class encapsulates every connection that process modules has
// with servers(data/render) or client. There are two basic types of
// connections:
// \li Self Connection: connections  between the root node and the 
// satellite processes.
// \li Remote Connection: connections between client and remote servers.

#ifndef __vtkProcessModuleConnection_h
#define __vtkProcessModuleConnection_h

#include "vtkObject.h"
#include "vtkClientServerID.h" // Needed for vtkClientServerID

class vtkClientServerStream;
class vtkCommand;
class vtkMultiProcessController;
class vtkProcessModuleConnectionObserver;
class vtkPVInformation;
class vtkPVProgressHandler;
class vtkPVXMLElement;

class VTK_EXPORT vtkProcessModuleConnection : public vtkObject
{
public:
  vtkTypeMacro(vtkProcessModuleConnection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  // Description:
  // Send a vtkClientServerStream to the specified servers. Servers
  // are specified with a bit vector.  To send to more than one server
  // use the bitwise or operator to combine servers.  The resetStream
  // flag determines if Reset is called to clear the stream after it
  // is sent.
  virtual int SendStream(vtkTypeUInt32 servers, vtkClientServerStream& stream);
//ETX

  // Description:
  // Initializes the connection. This is essential to
  // intialize the controller associated with the connection etc etc.
  // Subclasses must call this method to do some standard initialization if they
  // override this method.
  virtual int Initialize(int argc, char** argv, int *partitionId);

  // Description:
  // Finalizes the connection.
  virtual void Finalize();

  // Description:
  // Access the Controller used for this connection.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Get the number of processes participating in this connection.
  virtual int GetNumberOfPartitions();

  // Description:
  // Get the partition number. -1 means no assigned partition.
  virtual int GetPartitionId();

//BTX
  // Description:
  // Obtain the last result from the appropriate server.
  // Subclasses must override this. Default implementation flags an error.
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 serverFlags);

  // Description:
  // Gather information from the connection.
  // Subclasses must override this method. Default implementation
  // raises an error.
  virtual void GatherInformation(vtkTypeUInt32 serverFlags,
    vtkPVInformation* info,  vtkClientServerID id);
//ETX

  // Description:
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise. Subclass must override this. Default implementation merely
  // flags an error.
  virtual int LoadModule(const char* name, const char* directory);

  // Description:
  // When ever any irrecoverable communication errors are
  // detected, AbortConnection flag is set. 
  // vtkProcessModuleConnectionManager checks this flag
  // and closes dead connections.
  vtkGetMacro(AbortConnection, int);

  // Description:
  // Push the vtkUndoSet xml state on the undo stack for this connection.
  // Subclasses override this method to do the appropriate action.
  // On SelfConnection, the undo set is stored locally, while on
  // remote server connection, the undo set is sent to the server.
  // OBSOLETE: Will be deprecated soon.
  virtual void PushUndo(const char* vtkNotUsed(label),
    vtkPVXMLElement* vtkNotUsed(root)){}

  // Description:
  // Get the next undo  xml from this connection.
  // This method allocates  a new vtkPVXMLElement. It is the responsibility 
  // of caller to \c Delete it. 
  // \returns NULL on failure, otherwise the XML element is returned.
  // OBSOLETE: Will be deprecated soon.
  virtual vtkPVXMLElement* NewNextUndo(){return 0;}
 
  // Description:
  // Get the next redo  xml from this connection.
  // This method allocates  a new vtkPVXMLElement. It is the responsibility 
  // of caller to \c Delete it. 
  // \returns NULL on failure, otherwise the XML element is returned.
  // OBSOLETE: Will be deprecated soon.
  virtual vtkPVXMLElement* NewNextRedo(){return 0;}

  // Description:
  // Get the progress handler for this connection.
  vtkGetObjectMacro(ProgressHandler, vtkPVProgressHandler);

protected:
  vtkProcessModuleConnection();
  ~vtkProcessModuleConnection();

  // Description:
  // Given the servers that need to receive the stream, create a flag
  // that will send it to the correct places for this process module and
  // make sure it only gets sent to each server once.
  virtual vtkTypeUInt32 CreateSendFlag(vtkTypeUInt32 servers) =0;
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

  // handles callbacks.
  virtual void ExecuteEvent(vtkObject* caller, unsigned long eventId,
    void* calldata);

  virtual void OnSocketError();
  virtual void OnWrongTagEvent(vtkObject* caller, void* calldata);

  // Description:
  // Provide subclasses the access to the Observer.
  vtkCommand* GetObserver();

  vtkMultiProcessController* Controller;

  // Flag used to determine if the connection should be aborted. This
  // flag is set when SocketError occurs.
  int AbortConnection;
 
  vtkPVProgressHandler* ProgressHandler;
private:
  vtkProcessModuleConnection(const vtkProcessModuleConnection&); // Not implemented.
  void operator=(const vtkProcessModuleConnection&); // Not implemented.

  //BTX
  vtkProcessModuleConnectionObserver* Observer;
  friend class vtkProcessModuleConnectionObserver;
  //ETX
};

#endif

