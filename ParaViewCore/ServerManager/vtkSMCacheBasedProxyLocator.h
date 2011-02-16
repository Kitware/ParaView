/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCacheBasedProxyLocator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCacheBasedProxyLocator - is used to locate proxies which states
// have been previously stored inside the current instance.
// .SECTION Description
// This class can be used to keep track in memory of a given set of proxy state
// and allow them to be reset later on to that state. Underneath it use the
// XML state to prevent the burden of Sub-Proxy dependency management.

#ifndef __vtkSMCacheBasedProxyLocator_h
#define __vtkSMCacheBasedProxyLocator_h

#include "vtkSMProxyLocator.h"

class vtkSMStateLocator;
class vtkCollection;

class VTK_EXPORT vtkSMCacheBasedProxyLocator : public vtkSMProxyLocator
{
public:
  static vtkSMCacheBasedProxyLocator* New();
  vtkTypeMacro(vtkSMCacheBasedProxyLocator, vtkSMProxyLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Locate a proxy with the given "name". If none can be found returns NULL.
  // If a proxy with the name was not previously located, it will ask the
  // vtkSMStateLoader (if any) to re-new a proxy if possible and reset its state
  // to the previous stored XML one.
  vtkSMProxy* LocateProxy(vtkTypeUInt32 globalID);

  // Description:
  // Get/Set the StateLocator to used to rebuild unknown proxies
  // requested to be located using LocateProxy().
  void SetStateLocator(vtkSMStateLocator*);
  vtkGetObjectMacro(StateLocator, vtkSMStateLocator);

  // Description:
  // Clear the locator.
  void Clear();

  void GetLocatedProxies(vtkCollection*);

  void StoreProxyState(vtkSMProxy* proxy);

//BTX
protected:
  vtkSMCacheBasedProxyLocator();
  ~vtkSMCacheBasedProxyLocator();

  vtkSMStateLocator* StateLocator;

private:
  vtkSMCacheBasedProxyLocator(const vtkSMCacheBasedProxyLocator&); // Not implemented
  void operator=(const vtkSMCacheBasedProxyLocator&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
