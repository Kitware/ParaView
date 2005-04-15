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
// pointer(s) to vtkObject(s) (through vtkSMProxy). If 
// UpdateSelf is true, the proxy ids (as opposed to the server object ids)
// are passed to the stream. 
// Note: This property connects two proxies: proxy A (to which this property
// belongs) and proxy B (or more) (which is to be proxy A by using this 
// property).
// The way this is set depends on the number of IDs of the two proxies.
// If A and B have same number of IDs, the vtkObject represented by i'th ID on
// B is set on the server object represented by i'th ID on A. If A has 1 ID and
// B has more than one, than all IDs in B are set on A one after the other. 
// If B has 1 ID and A has more than one, than vtkObject represented by B is 
// set on all the server objects of A.
// 
// ProxyProperty supports attribute "remove_command". Note that if RemoveCommand 
// is set,  the clean_command is ignored. When RemoveCommand is set, only the 
// changes in the proxies (by AddProxy/RemoveProxy) are progaated to servers 
// .ie. those proxies not 
// present in the previous call to AppendCommandToStream are set on the 
// servers using this->Command
// and those missing during current call are removed from the servers using 
// this->RemoveCommand. Note that a property with "RemoveCommand" set should 
// not be shared among more than 1 proxies.
// 
//TODO: Update comment
// .SECTION See Also
// vtkSMProperty

#ifndef __vtkSMProxyProperty_h
#define __vtkSMProxyProperty_h

#include "vtkSMProperty.h"

class vtkSMProxy;
//BTX
struct vtkSMProxyPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProxyProperty : public vtkSMProperty
{
public:
  static vtkSMProxyProperty* New();
  vtkTypeRevisionMacro(vtkSMProxyProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a proxy to the list of proxies.
  int AddProxy(vtkSMProxy* proxy);
  void RemoveProxy(vtkSMProxy* proxy);
  int SetProxy(unsigned int idx, vtkSMProxy* proxy);

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
  int AddProxy(vtkSMProxy* proxy, int modify);

  // Description:
  // Removes a proxy from the vector of added Proxies (added by AddProxy).
  void RemoveProxy(vtkSMProxy* proxy, int modify);
  
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
  void AddUncheckedProxy(vtkSMProxy* proxy);
  void SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy);

  // Description:
  // Removes all unchecked proxies.
  void RemoveAllUncheckedProxies();

  // Description:
  // Remove all proxies from the list.
  void RemoveAllProxies();

  // Description:
  // Returns the number of proxies.
  unsigned int GetNumberOfProxies();

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
  virtual void DeepCopy(vtkSMProperty* src);

protected:
  vtkSMProxyProperty();
  ~vtkSMProxyProperty();

  //BTX
  // Description:
  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  // Note that if the proxy has multiple IDs, they are all appended to the 
  // command stream.  
  // All proxies added with AddProxy() will become "consumers" of
  // the proxy passed to AppendCommandToStream().
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  // Update all proxies referred by this property.
  virtual void UpdateAllInputs();

  // Description:
  // Saves the state of the object in XML format. 
  virtual void SaveState(const char* name,  ostream* file, vtkIndent indent);

  // Previous proxies are used by the ProxyProperty internally.
  // All proxies added with AddProxy() will become "consumers" of
  // the proxy passed to AppendCommandToStream().
  // Proxies that were added previous but that are no longer in the list
  // (removed) are no longer consumers of the proxy therefore RemoveProxy
  // is called on them. This requires keeping track of previous proxies.
  // Description:
  void AddPreviousProxy(vtkSMProxy* proxy);
  void RemoveAllPreviousProxies();

  // Description:
  // Given a proxy, remove all previous proxies from it's consumer list.
  void RemoveConsumers(vtkSMProxy* proxy);

  vtkSMProxyPropertyInternals* PPInternals;

  // Description:
  // Command that can be used to remove inputs. If set, this
  // command is called before the main Command is called with
  // all the arguments.
  vtkSetStringMacro(CleanCommand);
  vtkGetStringMacro(CleanCommand);
  char* CleanCommand;

  // Description:
  // Remove command is the command called to remove the VTK
  // object on the server-side. If set, CleanCommand is ignored.
  // Instead for every proxy that was absent from the proxies
  // previously pushed, the RemoveCommand is invoked.
  // NOTE: Do not share properties that have RemoveCommand set
  // among proxies, as they will not work. If required,
  // the support can be added.
  vtkSetStringMacro(RemoveCommand);
  vtkGetStringMacro(RemoveCommand);
  char* RemoveCommand;
  
  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

  vtkSetMacro(RepeatCommand, int);
  vtkGetMacro(RepeatCommand, int);
  int RepeatCommand;

  void AppendCommandToStreamWithRemoveCommand(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId );


  void AppendProxyToStream(vtkSMProxy* toAppend,
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId, int remove=0 );
private:
  vtkSMProxyProperty(const vtkSMProxyProperty&); // Not implemented
  void operator=(const vtkSMProxyProperty&); // Not implemented
};

#endif
