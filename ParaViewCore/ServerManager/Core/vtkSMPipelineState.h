/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPipelineState.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPipelineState
 * @brief   class that manage the state of the processing
 * pipeline
 *
 * This class is used to provide a RemoteObject API to the vtkSMProxyManager
 * which allow Undo/Redo and state sharing across several ParaView clients.
 * Basically, we expose the state management API of RemoteObject to handle
 * registration and unregistration of proxies.
*/

#ifndef vtkSMPipelineState_h
#define vtkSMPipelineState_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"          // needed for vtkSMMessage
#include "vtkSMRemoteObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkSMSession;
class vtkSMProxyLocator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMPipelineState : public vtkSMRemoteObject
{
  // My friends are...
  friend class vtkSMSessionProxyManager;

public:
  static vtkSMPipelineState* New();
  vtkTypeMacro(vtkSMPipelineState, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method return the full object state that can be used to create that
   * object from scratch.
   * This method will be used to fill the undo stack.
   * If not overridden this will return NULL.
   */
  const vtkSMMessage* GetFullState() override;

  /**
   * This method is used to initialise the object to the given state
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

  void ValidateState();

protected:
  /**
   * Default constructor.
   */
  vtkSMPipelineState();

  /**
   * Destructor.
   */
  ~vtkSMPipelineState() override;

private:
  vtkSMPipelineState(const vtkSMPipelineState&) = delete;
  void operator=(const vtkSMPipelineState&) = delete;
};

#endif // #ifndef vtkSMPipelineState_h
