/*=========================================================================

  Program:   ParaView
  Module:    vtkRMImplicitPlaneWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRMImplicitPlaneWidget 
// .SECTION Description

#ifndef __vtkRMImplicitPlaneWidget_h
#define __vtkRMImplicitPlaneWidget_h

#include "vtkRM3DWidget.h"

class VTK_EXPORT vtkRMImplicitPlaneWidget : public vtkRM3DWidget
{
public:
  static vtkRMImplicitPlaneWidget* New();
  vtkTypeRevisionMacro(vtkRMImplicitPlaneWidget, vtkRM3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  virtual void Create(vtkPVProcessModule *pm, 
    vtkClientServerID rendererID, vtkClientServerID interactorID);
 
  void SetCenter(double x, double y, double z);
  void GetCenter(double pts[3]);

  void SetNormal(double x, double y, double z);
  void GetNormal(double pts[3]);

  //BTX
  // Description:
  // The Tcl name of the VTK implicit plane.
  vtkGetMacro(PlaneID, vtkClientServerID);
  vtkSetMacro(PlaneID, vtkClientServerID);
  //ETX

  void ResetInternal();
  void UpdateVTKObject(vtkClientServerID objectID, char *variableName); 

  vtkGetVector3Macro(LastAcceptedCenter, double);
  vtkGetVector3Macro(LastAcceptedNormal, double);
  vtkSetVector3Macro(LastAcceptedCenter, double);
  vtkSetVector3Macro(LastAcceptedNormal, double);

  // Description:
  // Send a SetDrawPlane event to the server.
  void SetDrawPlane(int val);
protected:
  vtkRMImplicitPlaneWidget();
  ~vtkRMImplicitPlaneWidget();

  vtkClientServerID PlaneID;

  double LastAcceptedCenter[3];
  double LastAcceptedNormal[3];
  
  double Center[3];
  double Normal[3];

private:
  vtkRMImplicitPlaneWidget(const vtkRMImplicitPlaneWidget&); // Not implemented
  void operator=(const vtkRMImplicitPlaneWidget&); // Not implemented
};

#endif
