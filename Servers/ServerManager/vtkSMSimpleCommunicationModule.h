/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleCommunicationModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleCommunicationModule - simple implementation of communication module
// .SECTION Description
// vtkSMSimpleCommunicationModule is a very simple implementatation of 
// the communication module interface. It uses the existing vtkProcessModule
// to do the actual work. This class is temporary and will be replaced once
// the server manager is fully integrated into paraview (as opposed to only
// batch processing).

#ifndef __vtkSMSimpleCommunicationModule_h
#define __vtkSMSimpleCommunicationModule_h

#include "vtkSMCommunicationModule.h"

class vtkClientServerStream;
class vtkSocketController;

class VTK_EXPORT vtkSMSimpleCommunicationModule : public vtkSMCommunicationModule
{
public:
  static vtkSMSimpleCommunicationModule* New();
  vtkTypeRevisionMacro(vtkSMSimpleCommunicationModule,vtkSMCommunicationModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Make all nodes of the given server process the stream. The only
  // mode supported is paraview batch mode where node 0 is server
  // manager (server id 0) and all nodes are server (server id 1)
  virtual void SendStreamToServer(
    vtkClientServerStream* stream, int serverid);

  // Description:
  // Make all nodes of the given servers process the stream.
  // Even if a server is repeated in the list (either because there are
  // duplicate ids or two ids represent the same physical server),
  // the stream is sent only once to it.
  // Partially implemented.
  virtual void SendStreamToServers(
    vtkClientServerStream* stream, int numServers, const int* serverIDs);

  //BTX
  // Description:
  // Create, populate (by gathering) and copy an information object
  // from a server. See  vtkPVInformation for more information on
  // how information objects work.
  // Server id is ignored.
  virtual void GatherInformation(
    vtkPVInformation* info, vtkClientServerID id, int serverid) ;
  //ETX

protected:
  vtkSMSimpleCommunicationModule();
  ~vtkSMSimpleCommunicationModule();

private:
  vtkSMSimpleCommunicationModule(const vtkSMSimpleCommunicationModule&); // Not implemented
  void operator=(const vtkSMSimpleCommunicationModule&); // Not implemented
};

#endif
