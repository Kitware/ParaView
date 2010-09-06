/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompoundSourceProxy - a proxy excapsulation a pipeline of proxies.
// .SECTION Description
// vtkSMCompoundSourceProxy is a proxy that allows grouping of multiple proxies.
// vtkSMProxy has also this capability since a proxy can have sub-proxies.
// However, vtkSMProxy does not allow public access to these proxies. The
// only access is through exposed properties. The main reason behind this
// is consistency. There are proxies that will not work if the program
// accesses the sub-proxies directly. The main purpose of
// vtkSMCompoundSourceProxy is to provide an interface to access the
// sub-proxies. The compound proxy also maintains the connections between
// subproxies. This makes it possible to encapsulate a pipeline into a single
// proxy. Since vtkSMCompoundSourceProxy is a vtkSMSourceProxy, it can be
// directly used to input to other filters, representations etc.
// vtkSMCompoundSourceProxy provides API to exposed properties from sub proxies
// as well as output ports of the subproxies.


#ifndef __vtkSMCompoundSourceProxy_h
#define __vtkSMCompoundSourceProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMCompoundSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMCompoundSourceProxy* New();
  vtkTypeMacro(vtkSMCompoundSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMCompoundSourceProxy();
  ~vtkSMCompoundSourceProxy();

private:
  vtkSMCompoundSourceProxy(const vtkSMCompoundSourceProxy&); // Not implemented
  void operator=(const vtkSMCompoundSourceProxy&); // Not implemented
//ETX
};

#endif

