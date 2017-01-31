/*=========================================================================

  Program:   ParaView
  Module:    vtkSIInputProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIInputProperty
 *
 * ServerSide Property use to set vtkOutputPort as method parameter.
 * For that we need the object on which we should get the Port and its port
 * number.
*/

#ifndef vtkSIInputProperty_h
#define vtkSIInputProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProxyProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIInputProperty : public vtkSIProxyProperty
{
public:
  static vtkSIInputProperty* New();
  vtkTypeMacro(vtkSIInputProperty, vtkSIProxyProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Controls which input port this property uses when making connections.
   * By default, this is 0.
   */
  vtkGetMacro(PortIndex, int);
  //@}

protected:
  vtkSIInputProperty();
  ~vtkSIInputProperty();

  /**
   * Push a new state to the underneath implementation
   */
  virtual bool Push(vtkSMMessage*, int) VTK_OVERRIDE;

  /**
   * Parse the xml for the property.
   */
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) VTK_OVERRIDE;

  vtkSetMacro(PortIndex, int);
  int PortIndex;

private:
  vtkSIInputProperty(const vtkSIInputProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSIInputProperty&) VTK_DELETE_FUNCTION;
};

#endif
