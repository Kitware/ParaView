/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLineWidgetProxy
// .SECTION Description

#ifndef __vtkSMLineWidgetProxy_h
#define __vtkSMLineWidgetProxy_h

#include "vtkSM3DWidgetProxy.h"

class VTK_EXPORT vtkSMLineWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMLineWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMLineWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream &os,vtkIndent indent);

  void SetPoint1(double x, double y, double z);
  void SetPoint2(double x, double y, double z);
  vtkGetVector3Macro(Point1,double);
  vtkGetVector3Macro(Point2,double);

  void SetResolution(int res);
  vtkGetMacro(Resolution,int);

  virtual void SaveInBatchScript(ofstream *file);
protected:
//BTX
  vtkSMLineWidgetProxy();
  ~vtkSMLineWidgetProxy();

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
 
  virtual void CreateVTKObjects(int numObjects);
  
  double Point1[3];
  double Point2[3];
  int Resolution;
private:
  vtkSMLineWidgetProxy(const vtkSMLineWidgetProxy&);// Not implemented
  void operator=(const vtkSMLineWidgetProxy&); // Not implemented
//ETX
};  

#endif
