/*=========================================================================

  Program:   ParaView
  Module:    vtkSMConnectionCleanerProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMConnectionCleanerProxy
// .SECTION Description
// This is a proxy for any Connection Cleaner. Connection cleaners are 
// vtkSMObjects that are instantiated on any process (typically the root 
// node of the data server). They are setup to observer the termination
// of the connection on which the proxy is created. Once the connection
// terminated, they typically perform cleanup operations.
// Since the server side object needs to correctly the identify the connection
// on whose termination it should start the cleanup, we provide this proxy.
// on CreateVTKObjects(), this call SetConnectionID() method on the server
// side object with the server side conenctionID for the connection.
// We recollect that ConnectionIDs are not same among all processes. Every
// process i.e. (server/client/satellites) assign their own connection IDs
// for each connection.
// .SECTION See Also
// vtkSMServerSideAnimationPlayer

#ifndef __vtkSMConnectionCleanerProxy_h
#define __vtkSMConnectionCleanerProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMConnectionCleanerProxy : public vtkSMProxy
{
public:
  static vtkSMConnectionCleanerProxy* New();
  vtkTypeMacro(vtkSMConnectionCleanerProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMConnectionCleanerProxy();
  ~vtkSMConnectionCleanerProxy();

  // Description:
  // Overridden to set the connection ID correctly on the server side objects.
  virtual void CreateVTKObjects();
private:
  vtkSMConnectionCleanerProxy(const vtkSMConnectionCleanerProxy&); // Not implemented.
  void operator=(const vtkSMConnectionCleanerProxy&); // Not implemented.
};

#endif

