/*=========================================================================

  Program:   ParaView
  Module:    vtkRMLineWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRMLineWidget
// .SECTION Description

#ifndef __vtkRMLineWidget_h
#define __vtkRMLineWidget_h

#include "vtkRM3DWidget.h"

class VTK_EXPORT vtkRMLineWidget : public vtkRM3DWidget
{
public:
  static vtkRMLineWidget* New();
  vtkTypeRevisionMacro(vtkRMLineWidget, vtkRM3DWidget);
  void PrintSelf(ostream &os,vtkIndent indent);

 virtual void Create(vtkPVProcessModule *pm, 
    vtkClientServerID rendererID, vtkClientServerID interactorID);

 void SetPoint1(double x, double y, double z);
 void GetPoint1(double pt[3]);
 void SetPoint2(double x, double y, double z);
 void GetPoint2(double pt[3]);

 void SetResolution(int res);
 vtkGetMacro(Resolution,int);

 void UpdateVTKObject(vtkClientServerID objectID, char *point1Variable, 
   char *point2Variable, char *resolutionVariable);
 void ResetInternal();

 vtkGetVector3Macro(LastAcceptedPoint1,double);
 vtkGetVector3Macro(LastAcceptedPoint2,double);
 vtkSetVector3Macro(LastAcceptedPoint1,double);
 vtkSetVector3Macro(LastAcceptedPoint2,double);
 vtkGetMacro(LastAcceptedResolution,int);
 vtkSetMacro(LastAcceptedResolution,int);

protected:
  vtkRMLineWidget();
  ~vtkRMLineWidget();

  double LastAcceptedPoint1[3];
  double LastAcceptedPoint2[3];
  int LastAcceptedResolution;
  double Point1[3];
  double Point2[3];
  int Resolution;

private:
  vtkRMLineWidget(const vtkRMLineWidget&);// Not implemented
  void operator=(const vtkRMLineWidget&); // Not implemented
};  

#endif
