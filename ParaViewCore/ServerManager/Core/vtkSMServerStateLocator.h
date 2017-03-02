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
/**
 * @class   vtkSMServerStateLocator
 * @brief   Class used to retreive a given message state based
 * on its GlobalID from the DataServer.
 *
 * Retreive a given state from the server.
*/

#ifndef vtkSMServerStateLocator_h
#define vtkSMServerStateLocator_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"          // needed for vtkSMMessage.
#include "vtkSMStateLocator.h"
#include "vtkWeakPointer.h" // needed for the session ref

class vtkSMSession;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMServerStateLocator : public vtkSMStateLocator
{
public:
  static vtkSMServerStateLocator* New();
  vtkTypeMacro(vtkSMServerStateLocator, vtkSMStateLocator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get a parent locator to search which is used as a backup location
   * to search from if a given state was not found locally.
   */
  vtkSMSession* GetSession();
  void SetSession(vtkSMSession* session);
  //@}

  /**
   * Fill the provided State message with the state found inside the current
   * locator or one of its parent. The method return true if the state was
   * successfully filled.
   * In that case useParent is not used and is set to false.
   */
  virtual bool FindState(
    vtkTypeUInt32 globalID, vtkSMMessage* stateToFill, bool useParent) VTK_OVERRIDE;

protected:
  vtkSMServerStateLocator();
  ~vtkSMServerStateLocator();

  vtkWeakPointer<vtkSMSession> Session;

private:
  vtkSMServerStateLocator(const vtkSMServerStateLocator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMServerStateLocator&) VTK_DELETE_FUNCTION;
};

#endif
