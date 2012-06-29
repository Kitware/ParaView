/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCameraProxy - proxy for a camera.
// .SECTION Description
// This a proxy for a vtkCamera. This class optimizes UpdatePropertyInformation
// to use the client side object.

#ifndef __vtkSMCameraProxy_h
#define __vtkSMCameraProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMCameraProxy : public vtkSMProxy
{
public:
  static vtkSMCameraProxy* New();
  vtkTypeMacro(vtkSMCameraProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates all property informations by calling UpdateInformation()
  // and populating the values. It also calls UpdateDependentDomains()
  // on all properties to make sure that domains that depend on the
  // information are updated.
  virtual void UpdatePropertyInformation();
  virtual void UpdatePropertyInformation(vtkSMProperty* prop)
    { this->Superclass::UpdatePropertyInformation(prop); }
protected:
  vtkSMCameraProxy();
  ~vtkSMCameraProxy();

private:
  vtkSMCameraProxy(const vtkSMCameraProxy&); // Not implemented.
  void operator=(const vtkSMCameraProxy&); // Not implemented.
};

#endif

