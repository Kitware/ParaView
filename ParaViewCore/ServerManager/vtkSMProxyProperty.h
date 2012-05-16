/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyProperty - property representing pointer(s) to vtkObject(s)
// .SECTION Description
// vtkSMProxyProperty is a concrete sub-class of vtkSMProperty representing
// pointer(s) to vtkObject(s) (through vtkSMProxy).
//
// Besides the standard set of attributes, the following XML attributes are
// supported:
// \li command : identifies the method to call on the VTK object e.g.
// AddRepresentation.
// \li clean_command : if present, called once before invoking the method
// specified by \c command every time the property value is pushed e.g.
// RemoveAllRepresentations. If property
// can take multiple values then the \c command is called for for each of the
// values after the clean command for every push.
// \li remove_command : an alternative to clean_command where instead of
// resetting and adding all the values for every push, this simply calls the
// specified method to remove the vtk-objects no longer referred to e.g.
// RemoveRepresentation.
// \li argument_type : identifies the type for value passed to the method on the
// VTK object. Accepted values are "VTK", "SMProxy" or "SIProxy". Default is
// VTK.
// \li null_on_empty : if set to 1, whenever the property's value changes to
// empty i.e. it contains no proxies, the command is called on the VTK object
// with NULL argument useful when there's no clean_command that can be called on
// the VTK object to unset the property e.g. SetLookupTable(NULL).
// li skip_dependency : if set to 1, this property does not result in adding a
// dependency between the proxies set as values of this property and the proxy
// to which the property belongs (which is the default behaviour). Use this with
// care as it would mean that ParaView would no realize any updates are needed
// to the pipeline if any proxy set on the property changes. This is necessary
// in some cases, e.g. if LUT proxy on a representation changes, we don't want
// to representation to treat it same as if the input pipeline changed!
// .SECTION See Also
// vtkSMProperty

#ifndef __vtkSMProxyProperty_h
#define __vtkSMProxyProperty_h

#include "vtkSMProperty.h"

class vtkSMProxy;
class vtkSMStateLocator;
//BTX
struct vtkSMProxyPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProxyProperty : public vtkSMProperty
{
public:
  // Description:
  // When we load ProxyManager state we want Proxy/InputProperty to be able to
  // create the corresponding missing proxy. Although when the goal is to load
  // a state on any standard proxy, we do not want that proxy property be able
  // to create new proxy based on some previous state.
  static void EnableProxyCreation();
  static void DisableProxyCreation();
  static bool CanCreateProxy();

  static vtkSMProxyProperty* New();
  vtkTypeMacro(vtkSMProxyProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a proxy to the list of proxies.
  virtual int AddProxy(vtkSMProxy* proxy);
  virtual void RemoveProxy(vtkSMProxy* proxy);
  virtual int SetProxy(unsigned int idx, vtkSMProxy* proxy);

  // Description:
  // Sets the value of the property to the list of proxies specified.
  virtual void SetProxies(unsigned int numElements, 
    vtkSMProxy* proxies[]);

  // Description:
  // Returns if the given proxy is already added to the property.
  bool IsProxyAdded(vtkSMProxy* proxy);

  // Description:
  // Add a proxy to the list of proxies without calling Modified
  // (if modify is false). This is commonly used when ImmediateUpdate
  // is true but it is more efficient to avoid calling Update until
  // the last proxy is added. To do this, add all proxies with modify=false
  // and call Modified after the last.
  // This will perform domain checking. If the domain check fails,
  // the proxy will not be added and 0 will be returned.
  // Returns 1 on success. If the domain check fails or the property
  // is read only, returns 0.
  // All proxies added with AddProxy() will become "consumers" of
  // the proxy passed to AppendCommandToStream().
  virtual int AddProxy(vtkSMProxy* proxy, int modify);

  // Description:
  // Removes a proxy from the vector of added Proxies (added by AddProxy).
  // Returns the index of proxy removed. If the proxy was not found,
  // returns NumberOfProxies.
  virtual unsigned int RemoveProxy(vtkSMProxy* proxy, int modify);

  // Description:
  // Add an unchecked proxy. Does not modify the property.
  // Unchecked proxies are used by domains when verifying whether
  // a value is acceptable. To check if a value is in the domains,
  // you can do the following:
  // @verbatim
  // - RemoveAllUncheckedProxies()
  // - AddUncheckedProxy(proxy)
  // - IsInDomains()
  // @endverbatim
  virtual void AddUncheckedProxy(vtkSMProxy* proxy);
  virtual unsigned int RemoveUncheckedProxy(vtkSMProxy* proxy);
  virtual void SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy);

  // Description:
  // Removes all unchecked proxies.
  virtual void RemoveAllUncheckedProxies();

  virtual void ClearUncheckedProxies();

  // Description:
  // Remove all proxies from the list.
  virtual void RemoveAllProxies()
  {
    this->RemoveAllProxies(1);
  }

  // Description:
  // Sets the number of proxies. If the new number is greater than the current
  // number of proxies, then NULL will be inserted.
  virtual void SetNumberOfProxies(unsigned int num);

  // Description:
  // Returns the number of proxies.
  unsigned int GetNumberOfProxies();

  // Description:
  // Sets the number of unchecked proxies. If the new number is greater than the current
  // number of proxies, then NULL will be inserted.
  virtual void SetNumberOfUncheckedProxies(unsigned int num);

  // Description:
  // Returns the number of unchecked proxies.
  unsigned int GetNumberOfUncheckedProxies();

  // Description:
  // Return a proxy. No bounds check is performed.
  vtkSMProxy* GetProxy(unsigned int idx);

  // Description:
  // Return a proxy. No bounds check is performed.
  vtkSMProxy* GetUncheckedProxy(unsigned int idx);

  // Description: 
  // Copy all property values.
  virtual void Copy(vtkSMProperty* src);

  // Description:
  // Copy all proxies added to the src over to this by creating new 
  // instances for the proxies and inturn calling Copy to copy 
  // the proxies. exceptionClass and proxyPropertyCopyFlag are
  // used while copying over the values from the two proxy properties.
  virtual void DeepCopy(vtkSMProperty* src, const char* exceptionClass, 
    int proxyPropertyCopyFlag);

  // Description:
  // Returns whether the "skip_dependency" attribute is set.
  vtkGetMacro(SkipDependency, bool);

//BTX
protected:
  vtkSMProxyProperty();
  ~vtkSMProxyProperty();

  // Description:
  // Let the property write its content into the stream
  virtual void WriteTo(vtkSMMessage* msg);

  // Description:
  // Let the property read and set its content from the stream
  virtual void ReadFrom(const vtkSMMessage* msg, int msg_offset, vtkSMProxyLocator*);

  virtual void RemoveAllProxies(int modify);

  friend class vtkSMProxy;
  friend struct vtkSMProxyPropertyInternals;

  vtkSMProxyPropertyInternals* PPInternals;

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);


  // Description:
  // Generic method used to generate XML state
  virtual void SaveStateValues(vtkPVXMLElement* propertyElement);

  // Description:
  // Fill state property/proxy XML element with proxy info.
  // Return the created proxy XML element that has been added as a child in the
  // property definition. If prop == NULL, you must Delete yourself the result
  // otherwise prop is olding a reference to the proxy element
  virtual vtkPVXMLElement* AddProxyElementState(vtkPVXMLElement *prop,
                                                unsigned int idx);
  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader);

  // Static flag used to know if the locator should be used to create proxy
  // or if the session should be used to find only the existing ones
  static bool CreateProxyAllowed;

  bool SkipDependency;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&); // Not implemented
  void operator=(const vtkSMProxyProperty&); // Not implemented

  class vtkProxyPointer;
  friend class vtkSMProxyProperty::vtkProxyPointer;

  void AddProducer(vtkSMProxy*);
  void RemoveProducer(vtkSMProxy*);
//ETX
};

#endif
