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
/**
 * @class   vtkSMOrderedPropertyIterator
 * @brief   iterates over the properties of a proxy
 *
 * vtkSMOrderedPropertyIterator is used to iterate over the properties of a
 * proxy. The properties of the root proxies as well as sub-proxies are
 * included in the iteration. For sub-proxies, only exposed properties are
 * iterated over. vtkSMOrderedPropertyIterator iterates over properties in
 * the order they appear in the xml or in the order they were added. This
 * is possible because vtkSMProxy keeps track of the order in which properties
 * were added or exposed in a PropertyNamesInOrder vector.
*/

#ifndef vtkSMOrderedPropertyIterator_h
#define vtkSMOrderedPropertyIterator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMOrderedPropertyIterator : public vtkSMObject
{
public:
  static vtkSMOrderedPropertyIterator* New();
  vtkTypeMacro(vtkSMOrderedPropertyIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the proxy to be used.
   */
  void SetProxy(vtkSMProxy* proxy);

  //@{
  /**
   * Return the proxy.
   */
  vtkGetObjectMacro(Proxy, vtkSMProxy);
  //@}

  /**
   * Go to the first property.
   */
  void Begin();

  /**
   * Returns true if iterator points past the end of the collection.
   */
  int IsAtEnd();

  /**
   * Move to the next property.
   */
  void Next();

  /**
   * Returns the key (name) at the current iterator position.
   */
  const char* GetKey();

  /**
   * Returns the property at the current iterator position.
   */
  vtkSMProperty* GetProperty();

  /**
   * Returns the XMLLabel for self properties and the exposed name for
   * sub-proxy properties.
   */
  const char* GetPropertyLabel();

protected:
  vtkSMOrderedPropertyIterator();
  ~vtkSMOrderedPropertyIterator() override;

  vtkSMProxy* Proxy;
  unsigned int Index;

private:
  vtkSMOrderedPropertyIterator(const vtkSMOrderedPropertyIterator&) = delete;
  void operator=(const vtkSMOrderedPropertyIterator&) = delete;
};

#endif
