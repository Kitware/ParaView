/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIdBasedProxyLocator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIdBasedProxyLocator - vtkSMProxyLocator subclass that treats the
// id to be the SelfID of the proxy and locates proxy with the indicated SelfID.
// .SECTION Description
// vtkSMIdBasedProxyLocator is  a vtkSMProxyLocator subclass that treats the
// id to be the SelfID of the proxy and locates proxy with the indicated SelfID.

#ifndef __vtkSMIdBasedProxyLocator_h
#define __vtkSMIdBasedProxyLocator_h

#include "vtkSMProxyLocator.h"

class VTK_EXPORT vtkSMIdBasedProxyLocator : public vtkSMProxyLocator
{
public:
  static vtkSMIdBasedProxyLocator* New();
  vtkTypeMacro(vtkSMIdBasedProxyLocator, vtkSMProxyLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMIdBasedProxyLocator();
  ~vtkSMIdBasedProxyLocator();

  // Description:
  // Create new proxy with the given id. Default implementation asks the
  // Deserializer, if any, to create a new proxy.
  virtual vtkSMProxy* NewProxy(int id);

private:
  vtkSMIdBasedProxyLocator(const vtkSMIdBasedProxyLocator&); // Not implemented
  void operator=(const vtkSMIdBasedProxyLocator&); // Not implemented
//ETX
};

#endif

