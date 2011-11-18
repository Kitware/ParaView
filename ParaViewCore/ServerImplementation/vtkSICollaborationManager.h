/*=========================================================================

  Program:   ParaView
  Module:    vtkSICollaborationManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSICollaborationManager
// .SECTION Description
// Object that managed multi-client communication and provide the group awareness

#ifndef __vtkSICollaborationManager_h
#define __vtkSICollaborationManager_h

#include "vtkSIObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class VTK_EXPORT vtkSICollaborationManager : public vtkSIObject
{
public:
  static vtkSICollaborationManager* New();
  vtkTypeMacro(vtkSICollaborationManager, vtkSIObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Push a new state to the underneath implementation
  // The provided implementation just store the message
  // and return it at the Pull one.
  virtual void Push(vtkSMMessage* msg);

  // Description:
  // Pull the current state of the underneath implementation
  // The provided implementation update the given message with the one
  // that has been previously pushed
  virtual void Pull(vtkSMMessage* msg);

protected:
  vtkSICollaborationManager();
  virtual ~vtkSICollaborationManager();

  friend class vtkInternal;
  void BroadcastToClients(vtkSMMessage* msg);

private:
  vtkSICollaborationManager(const vtkSICollaborationManager&);    // Not implemented
  void operator=(const vtkSICollaborationManager&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif // #ifndef __vtkSICollaborationManager_h
