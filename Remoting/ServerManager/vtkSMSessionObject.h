/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSessionObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSessionObject
 * @brief   superclass for any server manager classes
 *                            that are related to a session
 *
 * vtkSMSessionObject provides methods to set and get the relative session
*/

#ifndef vtkSMSessionObject_h
#define vtkSMSessionObject_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"
#include <vtkWeakPointer.h> // Needed to keep track of the session

class vtkSMSession;
class vtkSMSessionProxyManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSessionObject : public vtkSMObject
{
public:
  static vtkSMSessionObject* New();
  vtkTypeMacro(vtkSMSessionObject, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the session on wihch this object exists.
   * Note that session is not reference counted.
   */
  virtual void SetSession(vtkSMSession*);
  virtual vtkSMSession* GetSession();
  //@}

  /**
   * Return the corresponding ProxyManager if any.
   */
  virtual vtkSMSessionProxyManager* GetSessionProxyManager();

protected:
  vtkSMSessionObject();
  ~vtkSMSessionObject() override;

  /**
   * Identifies the session id to which this object is related.
   */
  vtkWeakPointer<vtkSMSession> Session;

  /**
   *
   * Helper class designed to call session->PrepareProgress() in constructor and
   * session->CleanupPendingProgress() in destructor. To use this class, simply
   * create this class on the stack with the vtkSMObject instance as the
   * argument to the constructor.
   * @code
   * {
   *    ...
   *    vtkScopedMonitorProgress(this);
   *    ...
   * }
   * @endcode
   */
  class VTKREMOTINGSERVERMANAGER_EXPORT vtkScopedMonitorProgress
  {
    vtkSMSessionObject* Parent;

  public:
    vtkScopedMonitorProgress(vtkSMSessionObject* parent);
    ~vtkScopedMonitorProgress();
  };

private:
  vtkSMSessionObject(const vtkSMSessionObject&) = delete;
  void operator=(const vtkSMSessionObject&) = delete;
};

#endif
