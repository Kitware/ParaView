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
// .NAME vtkSMSimpleCommunicationModule
// .SECTION Description

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
  virtual void SendStreamToServer(
    vtkClientServerStream* stream, int serverid);

  // Description:
  virtual void SendStreamToServers(
    vtkClientServerStream* stream, int numServers, const int* serverIDs);

  // Description:
  void Connect();

  // Description:
  void Disconnect();

  //BTX
  // Description:
  virtual void GatherInformation(
    vtkPVInformation* info, vtkClientServerID id, int serverid) ;
  //ETX

protected:
  vtkSMSimpleCommunicationModule();
  ~vtkSMSimpleCommunicationModule();

  vtkSocketController* SocketController;

private:
  vtkSMSimpleCommunicationModule(const vtkSMSimpleCommunicationModule&); // Not implemented
  void operator=(const vtkSMSimpleCommunicationModule&); // Not implemented
};

#endif
