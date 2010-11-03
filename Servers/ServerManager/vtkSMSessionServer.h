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
// .NAME vtkSMSessionServer
// .SECTION Description
//

#ifndef __vtkSMSessionServer_h
#define __vtkSMSessionServer_h

#include "vtkSMSession.h"

class vtkMultiProcessController;
class vtkMultiProcessStream;

class VTK_EXPORT vtkSMSessionServer : public vtkSMSession
{
public:
  static vtkSMSessionServer* New();
  vtkTypeMacro(vtkSMSessionServer, vtkSMSession);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a ServerFlags indicate the nature of the current processes. e.g. if
  // the current processes acts as a data-server and a render-server, it returns
  // DATA_SERVER | RENDER_SERVER.
  // Overridden to return CLIENT since this process only acts as the client.
  virtual ServerFlags GetProcessRoles();

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

//BTX
  // Description:
  // Push the state.
  virtual void PushState(vtkSMMessage* msg);

  // Description:
  // Push the state.
  virtual void PullState(vtkSMMessage* msg);

  // Description:
  // Gather information about an object referred by the \c globalid.
  // \c location identifies the processes to gather the information from.
  // Overridden to fetch the information from server if needed, otherwise it's
  // handled locally.
  virtual bool GatherInformation(vtkTypeUInt32, vtkPVInformation*, vtkTypeUInt32)
    {
    vtkErrorMacro("GatherInformation cannot be called on the server processes.");
    return false;
    }


  // Description:
  // Called when client triggers GatherInformation().
  void GatherInformationInternal(
    vtkTypeUInt32 location, const char* classname, vtkTypeUInt32 globalid,
    vtkMultiProcessStream&);

  void OnClientServerMessageRMI(void* message, int message_length);
  void OnCloseSessionRMI();

protected:
  vtkSMSessionServer();
  ~vtkSMSessionServer();

  void SetClientController(vtkMultiProcessController*);

  vtkMultiProcessController* ClientController;
private:
  vtkSMSessionServer(const vtkSMSessionServer&); // Not implemented
  void operator=(const vtkSMSessionServer&); // Not implemented
//ETX
};

#endif
