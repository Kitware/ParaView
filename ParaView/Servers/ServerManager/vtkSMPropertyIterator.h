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
// .NAME vtkSMPropertyIterator - iterates over the properties of a proxy
// .SECTION Description
// vtkSMPropertyIterator is used to iterate over the properties of a
// proxy. The properties of the root proxies as well as sub-proxies are
// included in the iteration. For sub-proxies, only
// exposed properties are iterated over.

#ifndef __vtkSMPropertyIterator_h
#define __vtkSMPropertyIterator_h

#include "vtkSMObject.h"

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
  // Go to the first property.
  void Begin();

  // Description:
  // Returns true if iterator points past the end of the collection.
  int IsAtEnd();

  // Description:
  // Move to the next property.
  void Next();

  // Description:
  // Returns the key (name) at the current iterator position.
  const char* GetKey();

  // Description:
  // Returns the property at the current iterator position.
  vtkSMProperty* GetProperty();

  // Description:
  // If TraverseSubProxies is false, only the properties belonging
  // to the root proxy are returned. Default is true.
  vtkSetMacro(TraverseSubProxies, int);
  vtkGetMacro(TraverseSubProxies, int);

protected:
  vtkSMPropertyIterator();
  ~vtkSMPropertyIterator();

  vtkSMProxy* Proxy;

  int TraverseSubProxies;

  virtual void SaveState(const char*, ostream*, vtkIndent) {}

private:
  vtkSMPropertyIteratorInternals* Internals;

  vtkSMPropertyIterator(const vtkSMPropertyIterator&); // Not implemented
  void operator=(const vtkSMPropertyIterator&); // Not implemented
};

#endif
