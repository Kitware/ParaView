/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoxWidgetProxy
// .SECTION Description


#ifndef __vtkSMBoxWidgetProxy_h
#define __vtkSMBoxWidgetProxy_h

#include "vtkSM3DWidgetProxy.h"

class vtkTransform;
class vtkMatrix4x4;

class VTK_EXPORT vtkSMBoxWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMBoxWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMBoxWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Get/Set routines. These update the iVars, send messages to the
  //Server to update values and invoke WidgetMidifiedEvent which
  //can be observed to update the GUI values.
  void SetPosition(double x, double y, double z);
  double* GetPosition(); 
  void GetPosition(double &x, double &y, double&z);
  void GetPosition(double position[3]);
  
  //Description:
  //Get/Set routines. These update the iVars, send messages to the
  //Server to update values and invoke WidgetMidifiedEvent which
  //can be observed to update the GUI values.
  void SetRotation(double x, double y, double z);
  double* GetRotation();
  void GetRotation(double &x, double &y, double &z);
  void GetRotation(double rotation[3]);
  
  //Description:
  //Get/Set routines. These update the iVars, send messages to the
  //Server to update values and invoke WidgetMidifiedEvent which
  //can be observed to update the GUI values.
  void SetScale(double x, double y, double z);
  double* GetScale();
  void GetScale(double &x, double &y, double &z);
  void GetScale(double scale[3]);
  
  // Description:
  // Set/Get the transformation matrix.
  // Added to simplify Accept/Reset for the vtkPVBoxWidget
  //BTX
  void SetMatrix(vtkMatrix4x4* mat);
  void GetMatrix(vtkMatrix4x4* mat);
  void GetMatrix(double elements[16]); 
  //ETX
  void SetMatrix(double elements[16]);
  double* GetMatrix();
  //BTX
  vtkGetObjectMacro(BoxTransform,vtkTransform);

  void ResetInternal();
  void UpdateVTKObject();

  virtual void SaveInBatchScript(ofstream *file);
  //ETX
protected:
  //BTX
  vtkSMBoxWidgetProxy();
  ~vtkSMBoxWidgetProxy();

  void GetPositionInternal();
  void GetRotationInternal();
  void GetScaleInternal();

  void SetPositionNoEvent(double x, double y, double z);
  void SetScaleNoEvent(double x, double y, double z);
  void SetRotationNoEvent(double x, double y, double z);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  virtual void CreateVTKObjects(int numObjects); 

  vtkTransform*      BoxTransform;

  //Description:
  //Sends the updated transform matrix to the server.
  void Update();

  //Current iVars.
  double Position[3];
  double Rotation[3];
  double Scale[3];

  double Matrix[4][4];
private:
  vtkSMBoxWidgetProxy(const vtkSMBoxWidgetProxy&); // Not implemented
  void operator=(const vtkSMBoxWidgetProxy&); // Not implemented
  //ETX
};

#endif
