/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropertyIterator -
// .SECTION Description

#ifndef __vtkSMPropertyIterator_h
#define __vtkSMPropertyIterator_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

//BTX
struct vtkSMPropertyIteratorInternals;
//ETX

class vtkSMProperty;
class vtkSMProxy;

class VTK_EXPORT vtkSMPropertyIterator : public vtkSMObject
{
public:
  static vtkSMPropertyIterator* New();
  vtkTypeRevisionMacro(vtkSMPropertyIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the proxy to be used.
  void SetProxy(vtkSMProxy* proxy);

  // Description:
  // Return the proxy.
  vtkGetObjectMacro(Proxy, vtkSMProxy);

  // Description:
  void Begin();

  // Description:
  int IsAtEnd();

  // Description:
  void Next();

  // Description:
  const char* GetKey();

  // Description:
  vtkSMProperty* GetProperty();

protected:
  vtkSMPropertyIterator();
  ~vtkSMPropertyIterator();

  vtkSMProxy* Proxy;

private:
  vtkSMPropertyIteratorInternals* Internals;

  vtkSMPropertyIterator(const vtkSMPropertyIterator&); // Not implemented
  void operator=(const vtkSMPropertyIterator&); // Not implemented
};

#endif
