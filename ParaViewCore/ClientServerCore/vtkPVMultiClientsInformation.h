/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiClientsInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiClientsInformation - Gets Multi-clients informations from the server.
// .SECTION Description
// This objects is used by the client to get the number of multi-clients server
// as well as their ids.


#ifndef __vtkPVMultiClientsInformation_h
#define __vtkPVMultiClientsInformation_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTK_EXPORT vtkPVMultiClientsInformation : public vtkPVInformation
{
public:
  static vtkPVMultiClientsInformation* New();
  vtkTypeMacro(vtkPVMultiClientsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  void DeepCopy(vtkPVMultiClientsInformation *info);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Get the id that correspond to the current client
  vtkGetMacro(ClientId, int);

  // Description:
  // Return the client id of the nth connected client.
  // idx < NumberOfClients
  int GetClientId(int idx);

  // Description:
  // Return the number of connected clients
  vtkGetMacro(NumberOfClients, int);

  // Description:
  // Return 1 if the server allow server client to connect to itself
  vtkGetMacro(MultiClientEnable, int);

  // Description:
  // Return the Id of the client that has been elected as master
  vtkGetMacro(MasterId, int);

protected:
  vtkPVMultiClientsInformation();
  ~vtkPVMultiClientsInformation();

  int ClientId;
  int* ClientIds;
  int NumberOfClients;
  int MultiClientEnable;
  int MasterId;

  vtkPVMultiClientsInformation(const vtkPVMultiClientsInformation&); // Not implemented
  void operator=(const vtkPVMultiClientsInformation&); // Not implemented
};

#endif
