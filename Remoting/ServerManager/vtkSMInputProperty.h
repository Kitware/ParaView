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
/**
 * @class   vtkSMInputProperty
 * @brief   proxy representing inputs to a filter
 *
 * vtkSMInputProperty is a concrete sub-class of vtkSMProperty representing
 * inputs to a filter (through vtkSMProxy). It is a special property that
 * always calls AddInput on a vtkSMSourceProxy.
 * The xml configuration for input proxy supports the following attributes:
 * multiple_input: For an input port that connects multiple connections
 * such as the input of an append filter. port_index: The input port to
 * be used.
 * @sa
 * vtkSMInputProperty vtkSMSourceProxy
*/

#ifndef vtkSMInputProperty_h
#define vtkSMInputProperty_h

struct vtkSMInputPropertyInternals;

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxyProperty.h"

class vtkSMStateLocator;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMInputProperty : public vtkSMProxyProperty
{
public:
  static vtkSMInputProperty* New();
  vtkTypeMacro(vtkSMInputProperty, vtkSMProxyProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Should be set to true if the "input port" this property represents
   * can accept multiple inputs (for example, an append filter)
   */
  vtkSetMacro(MultipleInput, int);
  vtkGetMacro(MultipleInput, int);
  //@}

  //@{
  /**
   * Add a proxy to the list of input proxies. The outputPort controls
   * which outputPort will be used in connecting the pipeline.
   * The proxy is added with corresponding Add and Set methods and
   * can be removed with RemoveXXX() methods as usual.
   */
  void AddInputConnection(vtkSMProxy* proxy, unsigned int outputPort);
  void SetInputConnection(unsigned int idx, vtkSMProxy* proxy, unsigned int outputPort);
  //@}

  void AddUncheckedInputConnection(vtkSMProxy* proxy, unsigned int outputPort);
  void SetUncheckedInputConnection(unsigned int idx, vtkSMProxy* proxy, unsigned int inputPort);

  //@{
  /**
   * Sets the value of the property to the list of proxies specified.
   */
  virtual void SetProxies(
    unsigned int numElements, vtkSMProxy* proxies[], unsigned int outputports[]);
  using Superclass::SetProxies;
  //@}

  //@{
  /**
   * Given an index for a connection (proxy), returns which output port
   * is used to connect the pipeline.
   */
  unsigned int GetOutputPortForConnection(unsigned int idx);
  unsigned int GetUncheckedOutputPortForConnection(unsigned int idx);
  //@}

  //@{
  /**
   * Controls which input port this property uses when making connections.
   * By default, this is 0.
   */
  vtkSetMacro(PortIndex, int);
  vtkGetMacro(PortIndex, int);
  //@}

protected:
  vtkSMInputProperty();
  ~vtkSMInputProperty() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) override;

  /**
   * Fill state property/proxy XML element with output port attribute
   */
  vtkPVXMLElement* AddProxyElementState(
    vtkPVXMLElement* propertyElement, unsigned int idx) override;

  int MultipleInput;
  int PortIndex;

private:
  vtkSMInputProperty(const vtkSMInputProperty&) = delete;
  void operator=(const vtkSMInputProperty&) = delete;
};

#endif
