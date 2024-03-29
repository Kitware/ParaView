// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSICollaborationManager
 *
 * Object that managed multi-client communication and provide the group awareness
 */

#ifndef vtkSICollaborationManager_h
#define vtkSICollaborationManager_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSICollaborationManager : public vtkSIObject
{
public:
  static vtkSICollaborationManager* New();
  vtkTypeMacro(vtkSICollaborationManager, vtkSIObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Push a new state to the underneath implementation
   * The provided implementation just store the message
   * and return it at the Pull one.
   */
  void Push(vtkSMMessage* msg) override;

  /**
   * Pull the current state of the underneath implementation
   * The provided implementation update the given message with the one
   * that has been previously pushed
   */
  void Pull(vtkSMMessage* msg) override;

protected:
  vtkSICollaborationManager();
  ~vtkSICollaborationManager() override;

  friend class vtkInternal;
  void BroadcastToClients(vtkSMMessage* msg);

private:
  vtkSICollaborationManager(const vtkSICollaborationManager&) = delete;
  void operator=(const vtkSICollaborationManager&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif // #ifndef vtkSICollaborationManager_h
