/*=========================================================================

  Program:   ParaView
  Module:    vtkSelfConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelfConnection
// .SECTION Description
// This is superclass for connections that go to the same process 
// (or group of processes if running in MPI mode). This class keeps a 
// vtkMultiProcessController which coordinates comminications between
// this group of processes. This class provides an interface to to
// access this single process or group of processes without worrying about 
// the details of achieving that.

#ifndef __vtkSelfConnection_h
#define __vtkSelfConnection_h

#include "vtkProcessModuleConnection.h"

class vtkProcessModuleGUIHelper;
class vtkUndoStack;

class VTK_EXPORT vtkSelfConnection : public vtkProcessModuleConnection
{
public:
  static vtkSelfConnection* New();
  vtkTypeMacro(vtkSelfConnection, vtkProcessModuleConnection);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Initializes the connection. This is essential to
  // intialize the controller associated with the connection etc etc.
  virtual int Initialize(int argc, char** argv, int *partitionId); 

//BTX
  // Description:
  // Obtain the last result from the appropriate server. This override merely
  // sends the result from the local interpretor.
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 serverFlags);

  // Description:
  // vtkProcessModule::GatherInformation(with connection id) forwards the
  // call to this method on appropriate connection.
  virtual void GatherInformation(vtkTypeUInt32 serverFlags, 
    vtkPVInformation* info, vtkClientServerID id);
//ETX

  // Description:
  // Process the stream locally. Public to allow call in 
  // RMI callbacks.
  int ProcessStreamLocally(vtkClientServerStream& stream);
  int ProcessStreamLocally(unsigned char* data, int length);

  // Description:
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise. 
  virtual int LoadModule(const char* name, const char* directory);

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
  vtkSelfConnection();
  ~vtkSelfConnection();

  // Description:
  // Given the servers that need to receive the stream, create a flag
  // that will send it to the correct places for this process module and
  // make sure it only gets sent to each server once. 
  // This decides if to send the stream to root or everyone. 
  // Sending to all processes on a single process node is 
  // same as sending to the root.
  virtual vtkTypeUInt32 CreateSendFlag(vtkTypeUInt32 );

  // send a stream to the client.
  virtual int SendStreamToClient(vtkClientServerStream& s);

  vtkUndoStack* UndoRedoStack;

private:
  vtkSelfConnection(const vtkSelfConnection&); // Not implemented.
  void operator=(const vtkSelfConnection&); // Not implemented.
};


#endif

