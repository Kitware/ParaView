/*=========================================================================

  Program:   ParaView
  Module:    vtkRMPointWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRMPointWidget
// .SECTION Description

#ifndef __vtkRMPointWidget_h
#define __vtkRMPointWidget_h

#include "vtkRM3DWidget.h"

class VTK_EXPORT vtkRMPointWidget : public vtkRM3DWidget
{
public:
  static vtkRMPointWidget* New();
  vtkTypeRevisionMacro(vtkRMPointWidget, vtkRM3DWidget);
  void PrintSelf(ostream &os,vtkIndent indent);

 virtual void Create(vtkPVProcessModule *pm, 
    vtkClientServerID rendererID, vtkClientServerID interactorID);
  
 void GetPosition(double pt[3]);
 void SetPosition(double x, double y, double z);
 void UpdateVTKObject(vtkClientServerID ObjectID, char *VariableName);
 void ResetInternal();

 vtkGetVector3Macro(LastAcceptedPosition,double);
 vtkSetVector3Macro(LastAcceptedPosition,double);
protected:
  vtkRMPointWidget();
  ~vtkRMPointWidget();
  double LastAcceptedPosition[3];
  double Position[3];
  
private:
  vtkRMPointWidget(const vtkRMPointWidget&);// Not implemented
  void operator=(const vtkRMPointWidget&); // Not implemented
};

#endif
