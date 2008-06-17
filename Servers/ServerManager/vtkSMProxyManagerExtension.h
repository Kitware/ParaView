/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerExtension.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyManagerExtension - defines the interface for extensions to
// proxy manager.
// .SECTION Description
// vtkSMProxyManagerExtension defines the interface for extensions to the
// vtkSMProxyManager. One can register several concrete instances of
// vtkSMProxyManagerExtension subclasses with the proxy manager to extend the
// functionality.

#ifndef __vtkSMProxyManagerExtension_h
#define __vtkSMProxyManagerExtension_h

#include "vtkSMObject.h"
class vtkPVXMLElement;

class VTK_EXPORT vtkSMProxyManagerExtension : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMProxyManagerExtension, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is called when an extension is registered with the proxy manager to
  // ensure that the extension is compatible with the proxy manager version.
  // Incompatible extensions are not registered with the proxy manager.
  virtual bool CheckCompatibility(int major, int minor, int patch) = 0;

  // Description:
  // Given the proxy name and group name, returns the XML element for
  // the proxy.
  virtual vtkPVXMLElement* GetProxyElement(const char* groupName,
    const char* proxyName, vtkPVXMLElement* currentElement);

  // Description:
  // Used for serialization/de-serialization. This is need to revive the proxy
  // manager on the server side in case of saving animations after disconnect
  // etc.
  virtual vtkPVXMLElement* Save()
    { return 0; }
  virtual bool Load(vtkPVXMLElement* vtkNotUsed(elem))
    { return true; }

//BTX
protected:
  vtkSMProxyManagerExtension();
  ~vtkSMProxyManagerExtension();

private:
  vtkSMProxyManagerExtension(const vtkSMProxyManagerExtension&); // Not implemented
  void operator=(const vtkSMProxyManagerExtension&); // Not implemented
//ETX
};

#endif

