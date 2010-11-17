/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSessionClient
// .SECTION Description
// vtkSMSessionClient is a remote-session that connects to a remote server.
// vtkSMSessionClient supports both connecting a pvserver as well as connecting
// a pvdataserver/pvrenderserver.

#ifndef __vtkSMSessionClient_h
#define __vtkSMSessionClient_h

#include "vtkSMSession.h"

class vtkMultiProcessController;
class vtkPVServerInformation;

class VTK_EXPORT vtkSMSessionClient : public vtkSMSession
{
public:
  static vtkSMSessionClient* New();
  vtkTypeMacro(vtkSMSessionClient, vtkSMSession);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Returns true is this session is active/alive/valid.
  virtual bool GetIsAlive();

  // Description:
  // Returns a ServerFlags indicate the nature of the current processes. e.g. if
  // the current processes acts as a data-server and a render-server, it returns
  // DATA_SERVER | RENDER_SERVER.
  // Overridden to return CLIENT since this process only acts as the client.
  virtual ServerFlags GetProcessRoles() { return CLIENT; }

  // Description:
  // Returns the controller used to communicate with the process. Value must be
  // DATA_SERVER_ROOT or RENDER_SERVER_ROOT or CLIENT.
  virtual vtkMultiProcessController* GetController(ServerFlags processType);

//BTX
  // Description:
  // Push the state.
  virtual void PushState(vtkSMMessage* msg);
  virtual void PullState(vtkSMMessage* message);
  virtual void Invoke(vtkSMMessage* msg);

//ETX

  // Description:
  // When Connect() is waiting for a server to connect back to the client (in
  // reverse connect mode), then it periodically fires ProgressEvent.
  // Application can add observer to this signal and set this flag to true, if
  // it wants to abort the wait for the server.
  vtkSetMacro(AbortConnect, bool);

  // Description:
  // Gracefully exits the session.
  void CloseSession();

  // Description:
  // Gather information about an object referred by the \c globalid.
  // \c location identifies the processes to gather the information from.
  // Overridden to fetch the information from server if needed, otherwise it's
  // handled locally.
  virtual bool GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid);

  // Description:
  // Returns the number of processes on the given server/s. If more than 1
  // server is identified, than it returns the maximum number of processes e.g.
  // is servers = DATA_SERVER | RENDER_SERVER and there are 3 data-server nodes
  // and 2 render-server nodes, then this method will return 3.
  virtual int GetNumberOfProcesses(vtkTypeUInt32 servers);

//BTX
  enum {
    PUSH=1,
    INVOKE=2,
    PULL=3,
    GATHER_INFORMATION=4,
    CLIENT_SERVER_MESSAGE_RMI=55625,
    CLOSE_SESSION=55626,
    REPLY_GATHER_INFORMATION_TAG=55627,
    REPLY_PULL=55628
    };

protected:
  vtkSMSessionClient();
  ~vtkSMSessionClient();

  void SetRenderServerController(vtkMultiProcessController*);
  void SetDataServerController(vtkMultiProcessController*);

  void SetupDataServerRenderServerConnection();

  // Both maybe the same when connected to pvserver.
  vtkMultiProcessController* RenderServerController;
  vtkMultiProcessController* DataServerController;

  vtkPVServerInformation* DataServerInformation;
  vtkPVServerInformation* RenderServerInformation;

  bool AbortConnect;
private:
  vtkSMSessionClient(const vtkSMSessionClient&); // Not implemented
  void operator=(const vtkSMSessionClient&); // Not implemented
//ETX
};

#endif
