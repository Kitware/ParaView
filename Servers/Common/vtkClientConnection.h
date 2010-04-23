/*=========================================================================

  Program:   ParaView
  Module:    vtkClientConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientConnection - represent a connection with a client.
// .SECTION Description
// This is a remote connection "to" a Client. This class is only instantiated on 
// the server (data/render).

#ifndef __vtkClientConnection_h
#define __vtkClientConnection_h

#include "vtkRemoteConnection.h"
class vtkUndoStack;

class VTK_EXPORT vtkClientConnection : public vtkRemoteConnection
{
public:
  static vtkClientConnection* New();
  vtkTypeMacro(vtkClientConnection, vtkRemoteConnection);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Initializes the connection. This is essential to
  // intialize the controller associated with the connection etc etc.
  // This sets up the RMIs and returns. 
  virtual int Initialize(int argc, char** argv, int *partitionId); 
 
  // Description:
  // Finalizes the connection.
  virtual void Finalize();

  // Description:
  // Send the last result over to the client.
  void SendLastResult();

  // Description:
  // Gather information and send over to the Client.
  void SendInformation(vtkClientServerStream &stream);

  // Description:
  // Called when the server recieves a PushUndoSet request. Don't call
  // directly. Public so that the RMI callback can call it.
  void PushUndoXMLRMI(const char* label, const char* message);
  void UndoRMI();
  void RedoRMI();

  
  // Description:
  // Client connection does not support these method. Do nothing.
  virtual void PushUndo(const char*, vtkPVXMLElement*)  { }
  virtual vtkPVXMLElement* NewNextUndo() { return 0; }
  virtual vtkPVXMLElement* NewNextRedo() { return 0; }
//BTX
protected:
  vtkClientConnection();
  ~vtkClientConnection();

  // Description:
  // ClientConnection cannot send streams anywhere.
  virtual vtkTypeUInt32 CreateSendFlag(vtkTypeUInt32 ) { return 0;}

  // Description:
  // Authenticates with client. Returns 1 on success, 0 on error.
  int AuthenticateWithClient();

  // Description:
  // Set up RMI callbacks.
  void SetupRMIs();

  vtkUndoStack* UndoRedoStack;
  friend class vtkClientConnectionUndoSet;
  void SendRedoXML(const char* xml);
  void SendUndoXML(const char* xml);
private:
  vtkClientConnection(const vtkClientConnection&); // Not implemented.
  void operator=(const vtkClientConnection&); // Not implemented.
//ETX
};
                                       

#endif

