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
// .NAME vtkPVSession - extends vtkSession to add API for ParaView sessions.
// .SECTION Description
// vtkPVSession adds APIs to vtkSession for ParaView-specific sessions, namely
// those that are used to communicate between data-server,render-server and
// client. This is an abstract class.

#ifndef __vtkPVSession_h
#define __vtkPVSession_h

#include "vtkSession.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkPVSession : public vtkSession
{
public:
  vtkTypeMacro(vtkPVSession, vtkSession);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum ServerFlags
    {
    DATA_SERVER = 0x01,
    DATA_SERVER_ROOT = 0x02,
    RENDER_SERVER = 0x04,
    RENDER_SERVER_ROOT = 0x08,
    SERVERS = DATA_SERVER | RENDER_SERVER,
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
    };

  // Description:
  // Returns a ServerFlags indicate the nature of the current processes. e.g. if
  // the current processes acts as a data-server and a render-server, it returns
  // DATA_SERVER | RENDER_SERVER.
  virtual ServerFlags GetProcessRoles();

  // Description:
  // Returns the controller used to communicate with the process. Value must be
  // DATA_SERVER_ROOT or RENDER_SERVER_ROOT or CLIENT.
  // Default implementation returns NULL.
  virtual vtkMultiProcessController* GetController(ServerFlags processType);

//BTX
protected:
  vtkPVSession();
  ~vtkPVSession();

private:
  vtkPVSession(const vtkPVSession&); // Not implemented
  void operator=(const vtkPVSession&); // Not implemented
//ETX
};

#endif
