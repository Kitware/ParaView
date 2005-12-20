/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateLoader - Utility class to load state from XML
// .SECTION Description
// vtkSMStateLoader can load server manager state from a given 
// vtkPVXMLElement. This element is usually populated by a vtkPVXMLParser.
// .SECTION See Also
// vtkPVXMLParser vtkPVXMLElement

#ifndef __vtkSMStateLoader_h
#define __vtkSMStateLoader_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;

//BTX
struct vtkSMStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMStateLoader : public vtkSMObject
{
public:
  static vtkSMStateLoader* New();
  vtkTypeRevisionMacro(vtkSMStateLoader, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the state from the given root element. This root
  // element must have Proxy and ProxyCollection sub-elements
  // Returns 1 on success, 0 on failure.
  int LoadState(vtkPVXMLElement* rootElement);

  // Description:
  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key.
  vtkSMProxy* NewProxy(int id);

protected:
  vtkSMStateLoader();
  ~vtkSMStateLoader();

  vtkPVXMLElement* RootElement;

  int HandleProxyCollection(vtkPVXMLElement* collectionElement);

  vtkSMStateLoaderInternals* Internal;

private:
  vtkSMStateLoader(const vtkSMStateLoader&); // Not implemented
  void operator=(const vtkSMStateLoader&); // Not implemented
};

#endif
