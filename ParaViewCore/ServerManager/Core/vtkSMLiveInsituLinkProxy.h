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
// .NAME vtkSMLiveInsituLinkProxy
// .SECTION Description
//

#ifndef __vtkSMLiveInsituLinkProxy_h
#define __vtkSMLiveInsituLinkProxy_h

#include "vtkSMProxy.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class VTK_EXPORT vtkSMLiveInsituLinkProxy : public vtkSMProxy
{
public:
  static vtkSMLiveInsituLinkProxy* New();
  vtkTypeMacro(vtkSMLiveInsituLinkProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the a dummy proxy manager representing the 
  // insitu visualization pipeline.
  vtkSMSessionProxyManager* GetInsituProxyManager();

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

  void InsituConnected(const char* initialial_state);
  void NewTimestepAvailable();

  vtkSmartPointer<vtkSMSessionProxyManager> InsituProxyManager;

private:
  vtkSMLiveInsituLinkProxy(const vtkSMLiveInsituLinkProxy&); // Not implemented
  void operator=(const vtkSMLiveInsituLinkProxy&); // Not implemented
//ETX
};

#endif
