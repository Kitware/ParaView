/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteConnection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRemoteConnection - asbtract base class for connections that 
// need a socket.
// .SECTION Description
// This is an abstract superclass for connections that go over a socket
// i.e. those that use a SocketController. These kind of connections
// exist only on the Client or on the Server(Data/Render) Root node.

#ifndef __vtkRemoteConnection_h
#define __vtkRemoteConnection_h

#include "vtkProcessModuleConnection.h"

class vtkClientSocket;
class vtkSocketController;

class VTK_EXPORT vtkRemoteConnection : public vtkProcessModuleConnection
{
public:
  vtkTypeMacro(vtkRemoteConnection, vtkProcessModuleConnection);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  enum
    {
    CLIENT_SERVER_RMI_TAG = 938531,
    CLIENT_SERVER_ROOT_RMI_TAG = 938532,
    CLIENT_SERVER_LAST_RESULT_TAG = 838490,
    CLIENT_SERVER_GATHER_INFORMATION_RMI_TAG = 838491,
    CLIENT_SERVER_PUSH_UNDO_XML_TAG = 838494,

    CLIENT_SERVER_COMMUNICATION_TAG = 8843,
    ROOT_INFORMATION_LENGTH_TAG = 838492,
    ROOT_INFORMATION_TAG = 838493,
    ROOT_RESULT_LENGTH_TAG = 838487,
    ROOT_RESULT_TAG = 838488,
    UNDO_XML_TAG = 838495,
    REDO_XML_TAG = 838496
      
    };
//ETX
  
  // Description:
  // Set the "live" socket to use for this connection. This must be
  // set before Initialize() is called. The SocketController will 
  // use this socket for all the communication.
  // Returns 1 on success, 0 on failure.
  virtual int SetSocket(vtkClientSocket* soc);

  // Description:
  // Called to process some incoming data.
  // Returns 1 on success, 0 on failure.
  virtual int ProcessCommunication();

  // Description:
  // Get the socket controller used by this class.
  vtkSocketController* GetSocketController();

//BTX
  // Description:
  // These methods should be called before and after 
  // the subclass has sent some stream for processing on 
  // the SelfConnection.
  void Activate();
  void Deactivate();
protected:
  vtkRemoteConnection();
  ~vtkRemoteConnection(); 

private:
  vtkRemoteConnection(const vtkRemoteConnection&); // Not implemented.
  void operator=(const vtkRemoteConnection&); // Not implemented.
  
  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

