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
/**
 * @class   vtkSMPropertyIterator
 * @brief   iterates over the properties of a proxy
 *
 * vtkSMPropertyIterator is used to iterate over the properties of a
 * proxy. The properties of the root proxies as well as sub-proxies are
 * included in the iteration. For sub-proxies, only
 * exposed properties are iterated over.
*/

#ifndef vtkSMPropertyIterator_h
#define vtkSMPropertyIterator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

struct vtkSMPropertyIteratorInternals;

class vtkSMProperty;
class vtkSMProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPropertyIterator : public vtkSMObject
{
public:
  static vtkSMPropertyIterator* New();
  vtkTypeMacro(vtkSMPropertyIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the proxy to be used.
   */
  virtual void SetProxy(vtkSMProxy* proxy);

  //@{
  /**
   * Return the proxy.
   */
  vtkGetObjectMacro(Proxy, vtkSMProxy);
  //@}

  /**
   * Go to the first property.
   */
  virtual void Begin();

  /**
   * Returns true if iterator points past the end of the collection.
   */
  virtual int IsAtEnd();

  /**
   * Move to the next property.
   */
  virtual void Next();

  /**
   * Returns the key (name) at the current iterator position.
   */
  virtual const char* GetKey();

  /**
   * Returns the XMLLabel for self properties and the exposed name for
   * sub-proxy properties.
   */
  virtual const char* GetPropertyLabel();

  /**
   * Returns the property at the current iterator position.
   */
  virtual vtkSMProperty* GetProperty();

  //@{
  /**
   * If TraverseSubProxies is false, only the properties belonging
   * to the root proxy are returned. Default is true.
   */
  vtkSetMacro(TraverseSubProxies, int);
  vtkGetMacro(TraverseSubProxies, int);
  //@}

protected:
  vtkSMPropertyIterator();
  ~vtkSMPropertyIterator() override;

  vtkSMProxy* Proxy;

  int TraverseSubProxies;

private:
  vtkSMPropertyIteratorInternals* Internals;

  vtkSMPropertyIterator(const vtkSMPropertyIterator&) = delete;
  void operator=(const vtkSMPropertyIterator&) = delete;
};

#endif
