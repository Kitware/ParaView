/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerStateLocator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMServerStateLocator - Class used to retreive a given message state based
// on its GlobalID from the DataServer.
// .SECTION Description
// Retreive a given state from the server.

#ifndef __vtkSMServerStateLocator_h
#define __vtkSMServerStateLocator_h

#include "vtkSMStateLocator.h"
#include "vtkWeakPointer.h" // needed for the session ref
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage.

class vtkSMSession;

class VTK_EXPORT vtkSMServerStateLocator : public vtkSMStateLocator
{
public:
  static vtkSMServerStateLocator* New();
  vtkTypeMacro(vtkSMServerStateLocator, vtkSMStateLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get a parent locator to search which is used as a backup location
  // to search from if a given state was not found locally.
  vtkSMSession* GetSession();
  void SetSession(vtkSMSession* session);

//BTX
  // Description:
  // Fill the provided State message with the state found inside the current
  // locator or one of its parent. The method return true if the state was
  // successfully filled.
  // In that case useParent is not used and is set to false.
  virtual bool FindState(vtkTypeUInt32 globalID, vtkSMMessage* stateToFill,
                         bool useParent);

protected:
  vtkSMServerStateLocator();
  ~vtkSMServerStateLocator();

  vtkWeakPointer<vtkSMSession> Session;
private:
  vtkSMServerStateLocator(const vtkSMServerStateLocator&); // Not implemented
  void operator=(const vtkSMServerStateLocator&); // Not implemented

//ETX
};

#endif
