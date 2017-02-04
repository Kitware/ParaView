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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This method return the full object state that can be used to create that
   * object from scratch.
   * This method will be used to fill the undo stack.
   * If not overriden this will return NULL.
   */
  virtual const vtkSMMessage* GetFullState() VTK_OVERRIDE;

  /**
   * This method is used to initialise the object to the given state
   */
  virtual void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  void ValidateState();

protected:
  /**
   * Default constructor.
   */
  vtkSMPipelineState();

  /**
   * Destructor.
   */
  virtual ~vtkSMPipelineState();

private:
  vtkSMPipelineState(const vtkSMPipelineState&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPipelineState&) VTK_DELETE_FUNCTION;
};

#endif // #ifndef vtkSMPipelineState_h
