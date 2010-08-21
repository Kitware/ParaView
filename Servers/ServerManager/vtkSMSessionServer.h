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
#include "vtkSMMessage.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkSMSessionServer : public vtkSMSession
{
public:
  static vtkSMSessionServer* New();
  vtkTypeMacro(vtkSMSessionServer, vtkSMSession);
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
  // Push the state.
  virtual void PushState(vtkSMMessage* msg);

//BTX
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
