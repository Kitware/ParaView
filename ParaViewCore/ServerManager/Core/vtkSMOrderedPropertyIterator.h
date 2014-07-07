/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOrderedPropertyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMOrderedPropertyIterator - iterates over the properties of a proxy
// .SECTION Description
// vtkSMOrderedPropertyIterator is used to iterate over the properties of a
// proxy. The properties of the root proxies as well as sub-proxies are
// included in the iteration. For sub-proxies, only exposed properties are
// iterated over. vtkSMOrderedPropertyIterator iterates over properties in
// the order they appear in the xml or in the order they were added. This
// is possible because vtkSMProxy keeps track of the order in which properties
// were added or exposed in a PropertyNamesInOrder vector.

#ifndef __vtkSMOrderedPropertyIterator_h
#define __vtkSMOrderedPropertyIterator_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMOrderedPropertyIterator : public vtkSMObject
{
public:
  static vtkSMOrderedPropertyIterator* New();
  vtkTypeMacro(vtkSMOrderedPropertyIterator, vtkSMObject);
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
  // Returns the XMLLabel for self properties and the exposed name for
  // sub-proxy properties.
  const char* GetPropertyLabel();

protected:
  vtkSMOrderedPropertyIterator();
  ~vtkSMOrderedPropertyIterator();

  vtkSMProxy* Proxy;
  unsigned int Index;

private:
  vtkSMOrderedPropertyIterator(const vtkSMOrderedPropertyIterator&); // Not implemented
  void operator=(const vtkSMOrderedPropertyIterator&); // Not implemented
};

#endif
