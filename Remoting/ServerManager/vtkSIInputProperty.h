// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIInputProperty
 *
 * ServerSide Property use to set vtkOutputPort as method parameter.
 * For that we need the object on which we should get the Port and its port
 * number.
 */

#ifndef vtkSIInputProperty_h
#define vtkSIInputProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProxyProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIInputProperty : public vtkSIProxyProperty
{
public:
  static vtkSIInputProperty* New();
  vtkTypeMacro(vtkSIInputProperty, vtkSIProxyProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Controls which input port this property uses when making connections.
   * By default, this is 0.
   */
  vtkGetMacro(PortIndex, int);
  ///@}

protected:
  vtkSIInputProperty();
  ~vtkSIInputProperty() override;

  /**
   * Push a new state to the underneath implementation
   */
  bool Push(vtkSMMessage*, int) override;

  /**
   * Parse the xml for the property.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

  vtkSetMacro(PortIndex, int);
  int PortIndex;

private:
  vtkSIInputProperty(const vtkSIInputProperty&) = delete;
  void operator=(const vtkSIInputProperty&) = delete;
};

#endif
