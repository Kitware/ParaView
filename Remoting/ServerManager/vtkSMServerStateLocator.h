// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMServerStateLocator
 * @brief   Class used to retrieve a given message state based
 * on its GlobalID from the DataServer.
 *
 * Retrieve a given state from the server.
 */

#ifndef vtkSMServerStateLocator_h
#define vtkSMServerStateLocator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"            // needed for vtkSMMessage.
#include "vtkSMStateLocator.h"
#include "vtkWeakPointer.h" // needed for the session ref

class vtkSMSession;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMServerStateLocator : public vtkSMStateLocator
{
public:
  static vtkSMServerStateLocator* New();
  vtkTypeMacro(vtkSMServerStateLocator, vtkSMStateLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get a parent locator to search which is used as a backup location
   * to search from if a given state was not found locally.
   */
  vtkSMSession* GetSession();
  void SetSession(vtkSMSession* session);
  ///@}

  /**
   * Fill the provided State message with the state found inside the current
   * locator or one of its parent. The method return true if the state was
   * successfully filled.
   * In that case useParent is not used and is set to false.
   */
  bool FindState(vtkTypeUInt32 globalID, vtkSMMessage* stateToFill, bool useParent) override;

protected:
  vtkSMServerStateLocator();
  ~vtkSMServerStateLocator() override;

  vtkWeakPointer<vtkSMSession> Session;

private:
  vtkSMServerStateLocator(const vtkSMServerStateLocator&) = delete;
  void operator=(const vtkSMServerStateLocator&) = delete;
};

#endif
