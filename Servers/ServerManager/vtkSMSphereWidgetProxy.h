/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSphereWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSphereWidgetProxy
// .SECTION Description

#ifndef __vtkSMSphereWidgetProxy_h
#define __vtkSMSphereWidgetProxy_h


#include "vtkSM3DWidgetProxy.h"

class VTK_EXPORT vtkSMSphereWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMSphereWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMSphereWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream &os,vtkIndent indent);

  // Description:
  // Get/Set the Center
  void SetCenter(double x, double y, double z);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Get/Set the Radius
  void SetRadius(double radius);
  vtkGetMacro(Radius,double);

  virtual void SaveInBatchScript(ofstream *file);
protected:
  vtkSMSphereWidgetProxy();
  ~vtkSMSphereWidgetProxy();

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
  virtual void CreateVTKObjects(int numObjects); 

  double Center[3];
  double Radius;

private:
  vtkSMSphereWidgetProxy(const vtkSMSphereWidgetProxy&);// Not implemented
  void operator=(const vtkSMSphereWidgetProxy&); // Not implemented
};  

#endif
