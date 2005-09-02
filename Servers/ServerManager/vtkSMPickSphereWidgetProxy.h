/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickSphereWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPickSphereWidgetProxy
// .SECTION Description
// vtkSMPickSphereWidgetProxy is the proxy for vtkSphereWidget. 
// It maintains iVars for Center and Radius of the vtkSphereWidget.
// These values are pushed onto the vtkSphereWidget on
// UpdateVTKObjects(). 
#ifndef __vtkSMPickSphereWidgetProxy_h
#define __vtkSMPickSphereWidgetProxy_h

#include "vtkSMSphereWidgetProxy.h"

class VTK_EXPORT vtkSMPickSphereWidgetProxy : public vtkSMSphereWidgetProxy
{
public:
  static vtkSMPickSphereWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMPickSphereWidgetProxy, vtkSMSphereWidgetProxy);
  void PrintSelf(ostream &os,vtkIndent indent);

  // Description:
  // Called to push the values onto the VTK object.
  virtual void UpdateVTKObjects();

// ATTRIBUTE EDITOR 
  vtkSetMacro(MouseControlToggle,int);
  vtkGetMacro(MouseControlToggle,int);

protected:
  vtkSMPickSphereWidgetProxy();
  ~vtkSMPickSphereWidgetProxy();

// ATTRIBUTE EDITOR
  int MouseControlToggle;

private:
  vtkSMPickSphereWidgetProxy(const vtkSMPickSphereWidgetProxy&);// Not implemented
  void operator=(const vtkSMPickSphereWidgetProxy&); // Not implemented
};  

#endif
