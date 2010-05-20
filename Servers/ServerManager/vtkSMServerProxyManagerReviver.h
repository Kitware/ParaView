/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerProxyManagerReviver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMServerProxyManagerReviver - a server-side proxy manager reviver.
// .SECTION Description
// This class is a concrete implementation of a vtkSMProxyManagerReviver 
// which revives the ProxyManager on the server side. This only affects the 
// proxies that are on the connection that is being revived.
// The revival process is as follows:
// \li 1) Save ProxyManager state for the connection (cid).
// \li 2) Remove all displays/render modules from both the client and server.
// \li 3) Remove all other proxies (and their VTK objects) from the client side 
//    alone.
// \li 4) Ships the state to the server and loads the state on the 
//        server side proxy manager. New render modules/displays are created 
//        on the server, while other proxies are revived.
// To use this class, one simply instantiates an object on the client side
// and calls ReviveRemoteServerManager() with appropriate connection ID.
// To make use of the server side proxy manager one may typically use
// a ConnectionCleaner (vtkSMConnectionCleanerProxy).
// .SECTION See Also
// vtkSMConnectionCleanerProxy, vtkSMServerSideAnimationPlayer

#ifndef __vtkSMServerProxyManagerReviver_h
#define __vtkSMServerProxyManagerReviver_h

#include "vtkSMProxyManagerReviver.h"

class vtkPVXMLElement;
class VTK_EXPORT vtkSMServerProxyManagerReviver : public vtkSMProxyManagerReviver
{
public:
  static vtkSMServerProxyManagerReviver* New();
  vtkTypeMacro(vtkSMServerProxyManagerReviver, vtkSMProxyManagerReviver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method should cleanup the server manager state pertinant
  // to the connection \c cid and activate the server manager
  // on the remote process.
  virtual int ReviveRemoteServerManager(vtkIdType cid);

  // Description:
  // Internal method, don't call directly. When ReviveRemoteServerManager() is called
  // on the client side, this method is invoked on the server side object.
  int ReviveServerServerManager(const char* xmlstate, int maxid);

protected:
  vtkSMServerProxyManagerReviver();
  ~vtkSMServerProxyManagerReviver();

  void FilterStateXML(vtkPVXMLElement* root);
private:
  vtkSMServerProxyManagerReviver(const vtkSMServerProxyManagerReviver&); // Not implemented.
  void operator=(const vtkSMServerProxyManagerReviver&); // Not implemented.
};
#endif

