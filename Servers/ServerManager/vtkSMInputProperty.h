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
  // If InputsUpdateImmediately is true, all input properties push
  // their values as soon as they are modified. Otherwise, the values
  // are pushed UpdateVTKObjects
  static int GetInputsUpdateImmediately();
  static void SetInputsUpdateImmediately(int up);

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
  virtual void RemoveAllProxies()
  {
    this->Superclass::RemoveAllProxies();
  }

  // Description:
  // Sets the number of proxies. If the new number is greater than the current
  // number of proxies, then NULL will be inserted.
  virtual void SetNumberOfProxies(unsigned int num);

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

protected:
  vtkSMInputProperty();
  ~vtkSMInputProperty();

  virtual void RemoveAllProxies(int modify);

  unsigned int GetPreviousOutputPortForConnection(unsigned int idx);

  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, 
    vtkSMProxyLocator* loader, int loadLastPushedValues=0);

  // Description:
  // Called by ChildSaveState to save the XML for every proxy. Overridden to
  // save output port information.
  virtual vtkPVXMLElement* SaveProxyElementState(
    unsigned int idx, bool use_previous_proxies);

  //BTX
  // Description:
  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  // Note that if the proxy has multiple IDs, they are all appended to the 
  // command stream.  
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

  int MultipleInput;
  int PortIndex;

  vtkSMInputPropertyInternals* IPInternals;
  
  static int InputsUpdateImmediately;

private:
  vtkSMInputProperty(const vtkSMInputProperty&); // Not implemented
  void operator=(const vtkSMInputProperty&); // Not implemented
};

#endif
