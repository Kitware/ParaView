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
// .NAME vtkSMProxyProperty - property representing a pointer to vtkObject
// .SECTION Description
// vtkSMProxyProperty is a concrete sub-class of vtkSMProperty representing
// a pointer to a vtkObject (through vtkSMProxy). Note that if the proxy
// has multiple IDs, they are all appended to the command stream.
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
  void AddProxy(vtkSMProxy* proxy);
  void AddProxy(vtkSMProxy* proxy, int modify);

  // Description:
  void RemoveAllProxies();

  // Description:
  unsigned int GetNumberOfProxies();

  // Description:
  vtkSMProxy* GetProxy(unsigned int idx);

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
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  virtual void UpdateAllInputs();

  virtual void SaveState(const char* name,  ofstream* file, vtkIndent indent);

  vtkSMProxyPropertyInternals* PPInternals;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&); // Not implemented
  void operator=(const vtkSMProxyProperty&); // Not implemented
};

#endif
