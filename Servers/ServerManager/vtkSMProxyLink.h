/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyLink -
// .SECTION Description

#ifndef __vtkSMProxyLink_h
#define __vtkSMProxyLink_h

#include "vtkSMObject.h"

//BTX
class vtkCommand;
class vtkSMProxy;
struct vtkSMProxyLinkInternals;
//ETX

class VTK_EXPORT vtkSMProxyLink : public vtkSMObject
{
public:
  static vtkSMProxyLink* New();
  vtkTypeRevisionMacro(vtkSMProxyLink, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  void AddLinkedProxy(vtkSMProxy* proxy, int updateDir);

//BTX
  enum UpdateDirections
  {
    NONE = 0,
    INPUT = 1,
    OUTPUT = 2
  };
//ETX

protected:
  vtkSMProxyLink();
  ~vtkSMProxyLink();

//BTX
  friend class vtkSMProxyUpdateObserver;
//ETX

  void UpdateVTKObjects(vtkObject* caller);
  void UpdateProperties(vtkObject* caller, const char* pname);

  vtkCommand* Observer;

private:
  vtkSMProxyLinkInternals* Internals;

  vtkSMProxyLink(const vtkSMProxyLink&); // Not implemented
  void operator=(const vtkSMProxyLink&); // Not implemented
};

#endif
