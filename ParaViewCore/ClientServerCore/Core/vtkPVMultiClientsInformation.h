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
/**
 * @class   vtkPVMultiClientsInformation
 * @brief   Gets Multi-clients informations from the server.
 *
 * This objects is used by the client to get the number of multi-clients server
 * as well as their ids.
*/

#ifndef vtkPVMultiClientsInformation_h
#define vtkPVMultiClientsInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVMultiClientsInformation : public vtkPVInformation
{
public:
  static vtkPVMultiClientsInformation* New();
  vtkTypeMacro(vtkPVMultiClientsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void DeepCopy(vtkPVMultiClientsInformation* info);

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*) VTK_OVERRIDE;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Get the id that correspond to the current client
   */
  vtkGetMacro(ClientId, int);
  //@}

  /**
   * Return the client id of the nth connected client.
   * idx < NumberOfClients
   */
  int GetClientId(int idx);

  //@{
  /**
   * Return the number of connected clients
   */
  vtkGetMacro(NumberOfClients, int);
  //@}

  //@{
  /**
   * Return 1 if the server allow server client to connect to itself
   */
  vtkGetMacro(MultiClientEnable, int);
  //@}

  //@{
  /**
   * Return the Id of the client that has been elected as master
   */
  vtkGetMacro(MasterId, int);
  //@}

protected:
  vtkPVMultiClientsInformation();
  ~vtkPVMultiClientsInformation();

  int ClientId;
  int* ClientIds;
  int NumberOfClients;
  int MultiClientEnable;
  int MasterId;

  vtkPVMultiClientsInformation(const vtkPVMultiClientsInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVMultiClientsInformation&) VTK_DELETE_FUNCTION;
};

#endif
