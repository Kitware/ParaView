/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionCoreInterpreterHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVSessionCoreInterpreterHelper
 *
 * vtkPVSessionCoreInterpreterHelper is used by vtkPVSessionCore to avoid a
 * circular reference between the vtkPVSessionCore instance and its Interpreter.
*/

#ifndef vtkPVSessionCoreInterpreterHelper_h
#define vtkPVSessionCoreInterpreterHelper_h

#include "vtkObject.h"
#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkWeakPointer.h"                      // needed for vtkWeakPointer.

class vtkObject;
class vtkSIObject;
class vtkPVProgressHandler;
class vtkProcessModule;
class vtkPVSessionCore;
class vtkMPIMToNSocketConnection;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkPVSessionCoreInterpreterHelper : public vtkObject
{
public:
  static vtkPVSessionCoreInterpreterHelper* New();
  vtkTypeMacro(vtkPVSessionCoreInterpreterHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the vtkSIObject for the global-id. This is used by SIOBJECT() and
   * SIPROXY() stream (vtkClientServerStream) manipulator macros.
   */
  vtkSIObject* GetSIObject(vtkTypeUInt32 gid);

  /**
   * Returns the vtkObject corresponding to the global id. This is used by the
   * VTKOBJECT() stream (vtkClientServerStream) manipulator macros.
   */
  vtkObjectBase* GetVTKObject(vtkTypeUInt32 gid);

  /**
   * Reserve a global id block.
   */
  vtkTypeUInt32 GetNextGlobalIdChunk(vtkTypeUInt32 chunkSize);

  /**
   * Provides access to the process module.
   */
  vtkProcessModule* GetProcessModule();

  /**
   * Provides access to the progress handler.
   */
  vtkPVProgressHandler* GetActiveProgressHandler();

  /**
   * Sets and initializes the MPIMToNSocketConnection for communicating between
   * data-server and render-server.
   */
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection*);

  /**
   * Used by vtkPVSessionCore to pass the core. This is not reference counted.
   */
  void SetCore(vtkPVSessionCore*);

  //@{
  /**
   * Switch from 0:vtkErrorMacro to 1:vtkWarningMacro
   */
  vtkSetMacro(LogLevel, int);
  //@}

protected:
  vtkPVSessionCoreInterpreterHelper();
  ~vtkPVSessionCoreInterpreterHelper() override;

  vtkWeakPointer<vtkPVSessionCore> Core;
  int LogLevel;

private:
  vtkPVSessionCoreInterpreterHelper(const vtkPVSessionCoreInterpreterHelper&) = delete;
  void operator=(const vtkPVSessionCoreInterpreterHelper&) = delete;
};

#endif
