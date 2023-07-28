// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVMultiClientsInformation
 * @brief   Gets Multi-clients information from the server.
 *
 * This objects is used by the client to get the number of multi-clients server
 * as well as their ids.
 */

#ifndef vtkPVMultiClientsInformation_h
#define vtkPVMultiClientsInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

class vtkClientServerStream;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVMultiClientsInformation : public vtkPVInformation
{
public:
  static vtkPVMultiClientsInformation* New();
  vtkTypeMacro(vtkPVMultiClientsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void DeepCopy(vtkPVMultiClientsInformation* info);

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

  ///@{
  /**
   * Get the id that correspond to the current client
   */
  vtkGetMacro(ClientId, int);
  ///@}

  /**
   * Return the client id of the nth connected client.
   * idx < NumberOfClients
   */
  int GetClientId(int idx);

  ///@{
  /**
   * Return the number of connected clients
   */
  vtkGetMacro(NumberOfClients, int);
  ///@}

  ///@{
  /**
   * Return 1 if the server allow server client to connect to itself
   */
  vtkGetMacro(MultiClientEnable, int);
  ///@}

  ///@{
  /**
   * Return the Id of the client that has been elected as master
   */
  vtkGetMacro(MasterId, int);
  ///@}

protected:
  vtkPVMultiClientsInformation();
  ~vtkPVMultiClientsInformation() override;

  int ClientId;
  int* ClientIds;
  int NumberOfClients;
  int MultiClientEnable;
  int MasterId;

  vtkPVMultiClientsInformation(const vtkPVMultiClientsInformation&) = delete;
  void operator=(const vtkPVMultiClientsInformation&) = delete;
};

#endif
