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
// .NAME vtkSMLiveInsituLinkProxy - Performs additional operation on
// the Live client
// .SECTION Description
// Besides setting or getting parameters from its
// object (vtkSMLiveInsituLink) it receives paraview state from its
// object and applies it to the InsituProxyManager as well as sending
// modified paraview state to its object.
// @ingroup LiveInsitu

#ifndef __vtkSMLiveInsituLinkProxy_h
#define __vtkSMLiveInsituLinkProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPVCatalystSessionCore;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMLiveInsituLinkProxy : public vtkSMProxy
{
public:
  static vtkSMLiveInsituLinkProxy* New();
  vtkTypeMacro(vtkSMLiveInsituLinkProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the a dummy proxy manager representing the
  // insitu visualization pipeline.
  vtkSMSessionProxyManager* GetInsituProxyManager();
  void SetInsituProxyManager(vtkSMSessionProxyManager*);

  // Description:
  bool HasExtract(
    const char* reg_group, const char* reg_name, int port_number);

  // Description:
  vtkSMProxy* CreateExtract(
    const char* reg_group, const char* reg_name, int port_number);
  void RemoveExtract(vtkSMProxy*);
  // Description:
  // Wakes up Insitu side if simulation is paused. Handles corectly
  // several calls on the LIVE side.
  void LiveChanged();
  vtkIdType GetTimeStep()
  {
    return this->TimeStep;
  }

//BTX
  // Description:
  // Overridden to handle server-notification messages.
  virtual void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMLiveInsituLinkProxy();
  ~vtkSMLiveInsituLinkProxy();

  // overridden to ensure that we communicate the globalid for this proxy so
  // that the server-side can send messages to this proxy.
  virtual void CreateVTKObjects();

  void MarkStateDirty();

  // Description:
  // Pushes XML state to the server if needed.
  void PushUpdatedState();

  void InsituConnected(const char* initialial_state);
  void NextTimestepAvailable(vtkIdType timeStep);

  vtkSmartPointer<vtkSMSessionProxyManager> InsituProxyManager;
  vtkWeakPointer<vtkPVCatalystSessionCore> CatalystSessionCore;

  bool StateDirty;
  vtkIdType TimeStep;
private:
  vtkSMLiveInsituLinkProxy(const vtkSMLiveInsituLinkProxy&); // Not implemented
  void operator=(const vtkSMLiveInsituLinkProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
