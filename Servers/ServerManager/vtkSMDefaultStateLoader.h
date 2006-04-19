/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDefaultStateLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDefaultStateLoader
// .SECTION Description
// This is a vtkSMStateLoader subclass which uses the vtkProcessModule 
// singleton to locate proxies.
// .SECTION Notes
// Still debating is this class should disappear altogether and instead
// we add this functionality to vtkSMStateLoader. For now, keeping this class.

#ifndef __vtkSMDefaultStateLoader_h
#define __vtkSMDefaultStateLoader_h

#include "vtkSMStateLoader.h"

class VTK_EXPORT vtkSMDefaultStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMDefaultStateLoader* New();
  vtkTypeRevisionMacro(vtkSMDefaultStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Either create a new proxy or returns an existing one.
  // The vtkProcessModule singleton is used to check if the proxy exists.
  virtual vtkSMProxy* NewProxy(int id);
  virtual vtkSMProxy* NewProxyFromElement(vtkPVXMLElement* proxyElement, int id);
protected:
  vtkSMDefaultStateLoader();
  ~vtkSMDefaultStateLoader();

private:
  vtkSMDefaultStateLoader(const vtkSMDefaultStateLoader&); // Not implemented.
  void operator=(const vtkSMDefaultStateLoader&); // Not implemented.
};

#endif

