/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMInputProperty - proxy representing inputs to a filter
// .SECTION Description
// vtkSMInputProperty is a concrete sub-class of vtkSMProperty representing
// inputs to a filter (through vtkSMProxy). It is a special property that
// always calls AddInput on a vtkSMSourceProxy.
// The xml configuration for input proxy supports the following attributes:
// multiple_input: For an input port that connects multiple connections 
// such as the input of an append filter. port_index: The input port to
// be used.
// .SECTION See Also
// vtkSMInputProperty vtkSMSourceProxy

#ifndef __vtkSMInputProperty_h
#define __vtkSMInputProperty_h

//BTX
struct vtkSMInputPropertyInternals;
//ETX

#include "vtkSMProxyProperty.h"

class vtkSMStateLocator;

class VTK_EXPORT vtkSMInputProperty : public vtkSMProxyProperty
{
public:
  static vtkSMInputProperty* New();
  vtkTypeMacro(vtkSMInputProperty, vtkSMProxyProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Should be set to true if the "input port" this property represents
  // can accept multiple inputs (for example, an append filter)
  vtkSetMacro(MultipleInput, int);
  vtkGetMacro(MultipleInput, int);

  // Description:
  // Add a proxy to the list of input proxies. The outputPort controls
  // which outputPort will be used in connecting the pipeline.
  // The proxy is added with corresponding Add and Set methods and
  // can be removed with RemoveXXX() methods as usual.
  int AddInputConnection(vtkSMProxy* proxy, 
                         unsigned int outputPort,
                         int modify);
  int AddInputConnection(vtkSMProxy* proxy, 
                         unsigned int outputPort)
  {
    return this->AddInputConnection(proxy, outputPort, 1);
  }
  int SetInputConnection(unsigned int idx, 
                         vtkSMProxy* proxy, 
                         unsigned int inputPort);
  void AddUncheckedInputConnection(vtkSMProxy* proxy, 
                                   unsigned int outputPort);
  void SetUncheckedInputConnection(unsigned int idx, 
                                   vtkSMProxy* proxy, 
                                   unsigned int inputPort);

  // Description:
  // Overridden from superclass to also remove port. See superclass
  // for documentation.
  virtual void RemoveProxy(vtkSMProxy* proxy)
  {
    this->Superclass::RemoveProxy(proxy);
  }
  virtual unsigned int RemoveProxy(vtkSMProxy* proxy, int modify);
  virtual unsigned int RemoveUncheckedProxy(vtkSMProxy* proxy);
  virtual void RemoveAllUncheckedProxies();
  virtual void ClearUncheckedProxies();
  virtual void RemoveAllProxies()
  {
    this->Superclass::RemoveAllProxies();
  }

  // Description:
  // Sets the number of proxies. If the new number is greater than the current
  // number of proxies, then NULL will be inserted.
  virtual void SetNumberOfProxies(unsigned int num);
  virtual void SetNumberOfUncheckedProxies(unsigned int num);

  // Description:
  // Sets the value of the property to the list of proxies specified.
  virtual void SetProxies(unsigned int numElements, 
    vtkSMProxy* proxies[], unsigned int outputports[]);

  virtual void SetProxies(unsigned int numElements, 
    vtkSMProxy* proxies[])
    {
    this->Superclass::SetProxies(numElements, proxies);
    }

  // Description:
  // Given an index for a connection (proxy), returns which output port
  // is used to connect the pipeline.
  unsigned int GetOutputPortForConnection(unsigned int idx);
  unsigned int GetUncheckedOutputPortForConnection(unsigned int idx);

  // Description:
  // Controls which input port this property uses when making connections.
  // By default, this is 0.
  vtkSetMacro(PortIndex, int);
  vtkGetMacro(PortIndex, int);

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

protected:
  vtkSMInputProperty();
  ~vtkSMInputProperty();

  // Description:
  // Let the property write its content into the stream
  virtual void WriteTo(vtkSMMessage* msg);

  // Description:
  // Let the property read and set its content from the stream
  virtual void ReadFrom(const vtkSMMessage* msg, int message_offset,
                        vtkSMProxyLocator*);


  virtual void RemoveAllProxies(int modify);

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element);

  int MultipleInput;
  int PortIndex;

  vtkSMInputPropertyInternals* IPInternals;

  // Description:
  // Fill state property/proxy XML element with output port attribute
  virtual vtkPVXMLElement* AddProxyElementState(vtkPVXMLElement *propertyElement,
                                                unsigned int idx);

  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader);

private:
  vtkSMInputProperty(const vtkSMInputProperty&); // Not implemented
  void operator=(const vtkSMInputProperty&); // Not implemented
};

#endif
