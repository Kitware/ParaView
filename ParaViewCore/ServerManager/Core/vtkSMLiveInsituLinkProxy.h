/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMLiveInsituLinkProxy
 * @brief   Performs additional operation on
 * the Live client
 *
 * Besides setting or getting parameters from its
 * object (vtkSMLiveInsituLink) it receives paraview state from its
 * object and applies it to the InsituProxyManager as well as sending
 * modified paraview state to its object.
 * @ingroup LiveInsitu
*/

#ifndef vtkSMLiveInsituLinkProxy_h
#define vtkSMLiveInsituLinkProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include "vtkWeakPointer.h"  // needed for vtkWeakPointer.

class vtkPVCatalystSessionCore;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMLiveInsituLinkProxy : public vtkSMProxy
{
public:
  static vtkSMLiveInsituLinkProxy* New();
  vtkTypeMacro(vtkSMLiveInsituLinkProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Provides access to the a dummy proxy manager representing the
   * insitu visualization pipeline.
   */
  vtkSMSessionProxyManager* GetInsituProxyManager();
  void SetInsituProxyManager(vtkSMSessionProxyManager*);
  //@}

  bool HasExtract(const char* reg_group, const char* reg_name, int port_number);

  //@{
  vtkSMProxy* CreateExtract(const char* reg_group, const char* reg_name, int port_number);
  void RemoveExtract(vtkSMProxy*);
  //@}
  //@{
  /**
   * Wakes up Insitu side if simulation is paused. Handles correctly
   * several calls on the LIVE side.
   */
  void LiveChanged();
  vtkIdType GetTimeStep() { return this->TimeStep; }
  //@}

  /**
   * Overridden to handle server-notification messages.
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

  /**
   * Push updated states from the client to the server in aggregate (originally,
   * when the simulation was paused, multiple partial updates were pushed,
   * resulting in the connection between client and server to sever).
   */
  void PushUpdatedStates();

protected:
  vtkSMLiveInsituLinkProxy();
  ~vtkSMLiveInsituLinkProxy() override;

  // overridden to ensure that we communicate the globalid for this proxy so
  // that the server-side can send messages to this proxy.
  void CreateVTKObjects() override;

  void MarkStateDirty();

  /**
   * Pushes XML state to the server if needed.
   */
  void PushUpdatedState();

  void InsituConnected(const char* initialial_state);
  void NextTimestepAvailable(vtkIdType timeStep);

  vtkSmartPointer<vtkSMSessionProxyManager> InsituProxyManager;
  vtkWeakPointer<vtkPVCatalystSessionCore> CatalystSessionCore;

  bool StateDirty;
  vtkIdType TimeStep;

private:
  vtkSMLiveInsituLinkProxy(const vtkSMLiveInsituLinkProxy&) = delete;
  void operator=(const vtkSMLiveInsituLinkProxy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
