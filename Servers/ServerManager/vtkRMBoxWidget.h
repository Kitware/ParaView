/*=========================================================================

  Program:   ParaView
  Module:    vtkRMBoxWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRMBoxWidget
// .SECTION Description


#ifndef __vtkRMBoxWidget_h
#define __vtkRMBoxWidget_h

#include "vtkRM3DWidget.h"

class vtkTransform;
class vtkPlanes;

class VTK_EXPORT vtkRMBoxWidget : public vtkRM3DWidget
{
public:
  static vtkRMBoxWidget* New();
  vtkTypeRevisionMacro(vtkRMBoxWidget, vtkRM3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
  virtual void Create(vtkPVProcessModule *pm, 
    vtkClientServerID rendererID, vtkClientServerID interactorID);

  //Description:
  //Get/Set routines. These update the iVars, send messages to the
  //Server to update values and invoke WidgetMidifiedEvent which
  //can be observed to update the GUI values.
  void SetPosition(double x, double y, double z);
  void GetPosition(double pts[3]);

  //Description:
  //Get/Set routines. These update the iVars, send messages to the
  //Server to update values and invoke WidgetMidifiedEvent which
  //can be observed to update the GUI values.
  void SetRotation(double x, double y, double z);
  void GetRotation(double pts[3]);

  //Description:
  //Get/Set routines. These update the iVars, send messages to the
  //Server to update values and invoke WidgetMidifiedEvent which
  //can be observed to update the GUI values.
  void SetScale(double x, double y, double z);
  void GetScale(double pts[3]);

  //Description:
  //Separate Set routines for Scale/TrackWheel callbacks.
  //The diffenence is that these routines do not invoke the
  //WidgetModifiedEvent. Thus these rountines are to be called
  //when setting the iVars from GUI values.
  void SetPositionNoEvent(double x, double y, double z);
  void SetScaleNoEvent(double x, double y, double z);
  void SetRotationNoEvent(double x, double y, double z);

//BTX
  vtkSetVector3Macro(LastAcceptedPosition, double);
  vtkSetVector3Macro(LastAcceptedScale,    double);
  vtkSetVector3Macro(LastAcceptedRotation, double);
  vtkGetVector3Macro(LastAcceptedPosition, double);
  vtkGetVector3Macro(LastAcceptedScale,    double);
  vtkGetVector3Macro(LastAcceptedRotation, double);

  vtkGetMacro(BoxID,vtkClientServerID);
  vtkGetMacro(BoxTransformID,vtkClientServerID);
  vtkGetMacro(BoxMatrixID,vtkClientServerID);
  vtkGetObjectMacro(BoxTransform,vtkTransform);

  void ResetInternal();
  void UpdateVTKObject();
  virtual void PlaceWidget(double bds[6]);
//ETX
protected:
  vtkRMBoxWidget();
  ~vtkRMBoxWidget();

  vtkClientServerID BoxID;
  vtkClientServerID BoxTransformID;
  vtkClientServerID BoxMatrixID;

  vtkTransform*      BoxTransform;
  vtkPlanes*         Box;

  //Description:
  //Sends the updated transform matrix to the server.
  void Update();

  //Current iVars.
  double Position[3];
  double Rotation[3];
  double Scale[3];

  //Accepted iVars.
  double LastAcceptedPosition[3];
  double LastAcceptedRotation[3];
  double LastAcceptedScale[3];

private:
  vtkRMBoxWidget(const vtkRMBoxWidget&); // Not implemented
  void operator=(const vtkRMBoxWidget&); // Not implemented
};

#endif
