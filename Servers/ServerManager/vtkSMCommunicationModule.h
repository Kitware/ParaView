/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCommunicationModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCommunicationModule
// .SECTION Description

#ifndef __vtkSMCommunicationModule_h
#define __vtkSMCommunicationModule_h

#include "vtkSMObject.h"

#include "vtkClientServerID.h" // Needed for UniqueID ...

class vtkClientServerStream;
class vtkPVInformation;

class VTK_EXPORT vtkSMCommunicationModule : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMCommunicationModule, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual void SendStreamToServer(
    vtkClientServerStream* stream, int serverid) = 0;

  // Description:
  virtual void SendStreamToServers(
    vtkClientServerStream* stream, int numServers, int* serverIDs) = 0;

  // Description:
  vtkClientServerID GetUniqueID();

  // Description: 
  vtkClientServerID GetProcessModuleID()
    {
      vtkClientServerID id = {2};
      return id;
    }

  // Description:
  vtkClientServerID NewStreamObject(const char* type, 
                                    vtkClientServerStream& stream);

  //BTX
  // Description:
  void DeleteStreamObject(vtkClientServerID id, vtkClientServerStream& stream);

  // Description:
  virtual void GatherInformation(
    vtkPVInformation* info, vtkClientServerID id, int serverid) = 0;
  //ETX

protected:
  vtkSMCommunicationModule();
  ~vtkSMCommunicationModule();

  vtkClientServerID UniqueID;
    
private:
  vtkSMCommunicationModule(const vtkSMCommunicationModule&); // Not implemented
  void operator=(const vtkSMCommunicationModule&); // Not implemented
};

#endif
