/*=========================================================================

  Program:   ParaView
  Module:    vtkRMSphereWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRMSphereWidget
// .SECTION Description

#ifndef __vtkRMSphereWidget_h
#define __vtkRMSphereWidget_h


#include "vtkRM3DWidget.h"

class VTK_EXPORT vtkRMSphereWidget : public vtkRM3DWidget
{
public:
  static vtkRMSphereWidget* New();
  vtkTypeRevisionMacro(vtkRMSphereWidget, vtkRM3DWidget);
  void PrintSelf(ostream &os,vtkIndent indent);

 virtual void Create(vtkPVProcessModule *pm, 
    vtkClientServerID rendererID, vtkClientServerID interactorID);

 void SetCenter(double x, double y, double z);
 void GetCenter(double pts[3]);
 void SetRadius(double radius);
 vtkGetMacro(Radius,double);

 void ResetInternal();
 void UpdateVTKObject();
 
 vtkGetVector3Macro(LastAcceptedCenter,double);
 vtkSetVector3Macro(LastAcceptedCenter,double);
 vtkGetMacro(LastAcceptedRadius,double);
 vtkSetMacro(LastAcceptedRadius,double);

 vtkClientServerID GetSphereID() { return this->SphereID;}
protected:
  vtkRMSphereWidget();
  ~vtkRMSphereWidget();

  vtkClientServerID SphereID;
  double LastAcceptedCenter[3];
  double LastAcceptedRadius;
  double Center[3];
  double Radius;

private:
  vtkRMSphereWidget(const vtkRMSphereWidget&);// Not implemented
  void operator=(const vtkRMSphereWidget&); // Not implemented
};  

#endif
