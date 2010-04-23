/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialLoaderProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMaterialLoaderProxy - Proxy for the material loader helper.
// .SECTION Description
// This is the proxy for that helps in loading materials (shaders). To 
// load a material file, it must be present on both the render-server and the 
// client. This class ensures that the material file is present on the renderserver
// and the client. When in client servermode, the file from the client is 
// sent to all the render server nodes.

#ifndef __vtkSMMaterialLoaderProxy_h
#define __vtkSMMaterialLoaderProxy_h

#include "vtkSMProxy.h"
class vtkProperty;

class VTK_EXPORT vtkSMMaterialLoaderProxy : public vtkSMProxy
{
public:
  static vtkSMMaterialLoaderProxy* New();
  vtkTypeMacro(vtkSMMaterialLoaderProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the material. In non-client server mode, this simply sets the material
  // name on the property using vtkProperty::LoadMaterial(). 
  // In client server mode, if the material name is a file 
  // (which exists on the client), then the contents of the file are read and set 
  // on the vtkProperty using vtkProperty::LoadMaterialFromString(). If the materialname
  // cannot be located as a file on the client, then is simply sets the 
  // the materialname on the property using vtkProperty::LoadMaterial() assuming
  // that the material name is a library/repository material which each process
  // can locate for itself.
  void LoadMaterial(const char* materialname);

  void SetPropertyProxy(vtkSMProxy* proxy);
protected:
  vtkSMMaterialLoaderProxy();
  ~vtkSMMaterialLoaderProxy();
  
  vtkSMProxy* PropertyProxy;
private:
  vtkSMMaterialLoaderProxy(const vtkSMMaterialLoaderProxy&); // Not implemented.
  void operator=(const vtkSMMaterialLoaderProxy&); // Not implemented.
};


#endif

