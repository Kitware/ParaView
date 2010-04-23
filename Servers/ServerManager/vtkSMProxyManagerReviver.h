/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerReviver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyManagerReviver - abstract superclass that defines the 
// API for classes that can "revive" a proxy manager. 
// .SECTION Description
// vtkSMProxyManagerReviver defines the API for classes that can revive 
// the proxy manager. This involves two proxy managers, typically one each 
// on the server and the client side. To revive a proxy manager, implies
// shutting down the proxy manager on one side, and activating another one.

#ifndef __vtkSMProxyManagerReviver_h
#define __vtkSMProxyManagerReviver_h

#include "vtkSMObject.h"

class VTK_EXPORT vtkSMProxyManagerReviver : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMProxyManagerReviver, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method should cleanup the server manager state pertinant
  // to the connection \c cid and activate the server manager
  // on the remote process.
  virtual int ReviveRemoteServerManager(vtkIdType cid)=0;

protected:
  vtkSMProxyManagerReviver();
  ~vtkSMProxyManagerReviver();

private:
  vtkSMProxyManagerReviver(const vtkSMProxyManagerReviver&); // Not implemented.
  void operator=(const vtkSMProxyManagerReviver&); // Not implemented.
};


#endif

