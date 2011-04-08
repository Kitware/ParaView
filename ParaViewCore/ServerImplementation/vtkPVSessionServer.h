/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionServer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSessionServer
// .SECTION Description
// vtkSMSessionServer is a session used on data and/or render servers. It's
// designed for a process that works with a separate client process that acts as
// the visualization driver.
// .SECTION See Also
// vtkSMSessionClient

#ifndef __vtkPVSessionServer_h
#define __vtkPVSessionServer_h

#include "vtkPVSessionBase.h"

class vtkMultiProcessController;
class vtkMultiProcessStream;

class VTK_EXPORT vtkPVSessionServer : public vtkPVSessionBase
{
public:
  static vtkPVSessionServer* New();
  vtkTypeMacro(vtkPVSessionServer, vtkPVSessionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the controller used to communicate with the process. Value must be
  // DATA_SERVER_ROOT or RENDER_SERVER_ROOT or CLIENT.
  virtual vtkMultiProcessController* GetController(ServerFlags processType);

  // Description:
  // Connects a remote server. URL can be of the following format:
  // cs://<pvserver-host>:<pvserver-port>
  // cdsrs://<pvdataserver-host>:<pvdataserver-port>/<pvrenderserver-host>:<pvrenderserver-port>
  // In both cases the port is optional. When not provided default
  // pvserver/pvdataserver port // is 11111, while default pvrenderserver port
  // is 22221.
  // For reverse connect i.e. the client waits for the server to connect back,
  // simply add "rc" to the protocol e.g.
  // csrc://<pvserver-host>:<pvserver-port>
  // cdsrsrc://<pvdataserver-host>:<pvdataserver-port>/<pvrenderserver-host>:<pvrenderserver-port>
  // In this case, the hostname is irrelevant and is ignored.
  virtual bool Connect(const char* url);

  // Description:
  // Overload that constructs the url using the command line parameters
  // specified and then calls Connect(url).
  bool Connect();

  // Description:
  // Returns true is this session is active/alive/valid.
  virtual bool GetIsAlive();

  // Description:
  // Client-Server Communication tags.
  enum {
    PUSH=1,
    EXECUTE_STREAM=2,
    PULL=3,
    GATHER_INFORMATION=4,
    DELETE_SI=5,
    LAST_RESULT=6,
    CLIENT_SERVER_MESSAGE_RMI=55625,
    CLOSE_SESSION=55626,
    REPLY_GATHER_INFORMATION_TAG=55627,
    REPLY_PULL=55628,
    REPLY_LAST_RESULT=55629,
    EXECUTE_STREAM_TAG=55630,
  };


//BTX
  void OnClientServerMessageRMI(void* message, int message_length);
  void OnCloseSessionRMI();

protected:
  vtkPVSessionServer();
  ~vtkPVSessionServer();

  void SetClientController(vtkMultiProcessController*);

  // Description:
  // Called when client triggers GatherInformation().
  void GatherInformationInternal(
    vtkTypeUInt32 location, const char* classname, vtkTypeUInt32 globalid,
    vtkMultiProcessStream&);

  // Description:
  // Sends the last result to client.
  void SendLastResultToClient();

  vtkMultiProcessController* ClientController;
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;

  unsigned long ActivateObserverId;
  unsigned long DeActivateObserverId;
private:
  vtkPVSessionServer(const vtkPVSessionServer&); // Not implemented
  void operator=(const vtkPVSessionServer&); // Not implemented
//ETX
};

#endif
