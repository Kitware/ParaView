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
// .NAME vtkSMCommunicationModule - abstract superclass for communication modules
// .SECTION Description
// vtkSMCommunicationModule defines an abstract interface as well as some
// common functionality for all communication modules. Communication
// modules implement the communication layer between server manager and
// servers. They abstract the physical layout by assigning virtual ids
// to servers. The server manager's id is always 0.

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
  // Make all nodes of the given server process the stream.
  virtual void SendStreamToServer(
    vtkClientServerStream* stream, int serverid) = 0;

  // Description:
  // Make all nodes of the given servers process the stream.
  // Even if a server is repeated in the list (either because there are
  // duplicate ids or two ids represent the same physical server),
  // the stream is sent only once to it.
  virtual void SendStreamToServers(
    vtkClientServerStream* stream, int numServers, const int* serverIDs) = 0;

  //BTX
  // Description:
  // Returns a unique id.
  vtkClientServerID GetUniqueID();

  // Description:
  // Append the command to create a new object of given type
  // to the stream. Returns the id that WILL be assigned. 
  // This function does not process the stream. The object will not 
  // be created until the stream is processed (and the id will 
  // not be valid).
  vtkClientServerID NewStreamObject(const char* type, 
                                    vtkClientServerStream& stream);

  // Description:
  // Append the command to delete an existing object to the stream.
  // This function does not process the stream.
  // The object will be deleted only when the stream is processed.
  void DeleteStreamObject(vtkClientServerID id, vtkClientServerStream& stream);

  // Description:
  // Create, populate (by gathering) and copy an information object
  // from a server. See  vtkPVInformation for more information on
  // how information objects work.
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
