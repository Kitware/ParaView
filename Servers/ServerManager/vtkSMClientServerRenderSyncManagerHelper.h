/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientServerRenderSyncManagerHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMClientServerRenderSyncManagerHelper
// .SECTION Description
// This is a helper class to avoid code duplication between
// vtkSMIceTDesktopRenderViewProxy and vtkSMClientServerRenderViewProxy.
// It defines a bunch of static methods used by both.

#ifndef __vtkSMClientServerRenderSyncManagerHelper_h
#define __vtkSMClientServerRenderSyncManagerHelper_h

#include "vtkSMObject.h"

class vtkSMProxy;
struct vtkClientServerID;
class VTK_EXPORT vtkSMClientServerRenderSyncManagerHelper : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMClientServerRenderSyncManagerHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  // Description:
  // Creates the render window proxy. If sharedServerRenderWindowID is non-null,
  // then the VTK object on the server referred by the proxy is set to the
  // object pointed by the sharedServerRenderWindowID.
  static void CreateRenderWindow(vtkSMProxy* renWinProxy, 
    vtkClientServerID sharedServerRenderWindowID);

  // Description:
  // Creates the render sync manager. If sharedServerRSMID is non-null, then the
  // server side VTK object for the render sync manager is set to the one
  // pointed by the sharedServerRSMID. If sharedServerRSMID is null, then a new
  // instance of rsmServerClassName will be created on the server.
  static void CreateRenderSyncManager(vtkSMProxy* rsmProxy,
    vtkClientServerID sharedServerRSMID,
    const char* rsmServerClassName);

  // Description:
  // Initializes the render sync manager.
  static void InitializeRenderSyncManager(vtkSMProxy* rsmProxy,
    vtkSMProxy* renderWindowProxy);

protected:
  vtkSMClientServerRenderSyncManagerHelper();
  ~vtkSMClientServerRenderSyncManagerHelper();

private:
  vtkSMClientServerRenderSyncManagerHelper(const vtkSMClientServerRenderSyncManagerHelper&); // Not implemented
  void operator=(const vtkSMClientServerRenderSyncManagerHelper&); // Not implemented
//ETX
};

#endif

