/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBoxWidget - A widget to manipulate a box.
// .SECTION Description
// This widget creates and manages its own vtkPlanes on each process.
// I could not decide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVBoxWidget_h
#define __vtkPVBoxWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;
class vtkKWThumbWheel;
class vtkKWScale;
class vtkTransform;
class vtkBoxWidget;
class vtkPlanes;
class vtkXMProxy;

class VTK_EXPORT vtkPVBoxWidget : public vtkPV3DWidget
{
public:
  static vtkPVBoxWidget* New();
  vtkTypeRevisionMacro(vtkPVBoxWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVBoxWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // These are callbacks for the transformation of the box.
  void ScaleCallback();
  void TranslateCallback();
  void OrientationCallback();
  void ScaleEndCallback();
  void TranslateEndCallback();
  void OrientationEndCallback();
  void ScaleKeyPressCallback();
  void TranslateKeyPressCallback();
  void OrientationKeyPressCallback();

  // Description:
  // Set the box
  void SetScale(double p[3]){ this->SetScale(p[0], p[1], p[2]); }
  void SetScale(double px, double py, double pz);
  void SetScaleInternal(double x, double y, double z);
  void SetScaleInternal(double p[3]){this->SetScaleInternal(p[0],p[1],p[2]);}

  void SetTranslateInternal(double x, double y, double z);
  void SetTranslateInternal(double p[3]){this->SetTranslateInternal(p[0],p[1],p[2]);}
  void SetTranslate(double p[3]){ this->SetTranslate(p[0], p[1], p[2]); }
  void SetTranslate(double px, double py, double pz);

  void SetOrientationInternal(double px, double py, double pz);
  void SetOrientationInternal(double p[3]){this->SetOrientationInternal(p[0],p[1],p[2]);}
  void SetOrientation(double p[3]){ this->SetOrientation(p[0], p[1], p[2]); }
  void SetOrientation(double px, double py, double pz);

  //BTX
  vtkBoxWidget* GetBoxWidget();
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Provide access to the proxies used by this widget.
  // BoxTransform == BoxTransformProxy
  // BoxMatrix == BoxMatrixProxy
  // Box == BoxProxy
  // BoxPoints == BoxPointsProxy
  // BoxNormals == BoxNormalsProxy
  vtkSMProxy* GetProxyByName(const char* name);
  
protected:
  vtkPVBoxWidget();
  ~vtkPVBoxWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  void UpdateVTKObject(const char* sourceTclName);

  // Description:
  // Get iVar values from vtkRMBoxWidget object and update the GUI.
  void UpdateFromBox();

  // Description:
  // Enables/Disables invoking of callbacks of the ThumbWheel/Scale.
  // It makes sense to disable callbacks (which are responsible to update
  // the internal state of the widget) when the ThumbWheel/Scale values 
  // are being updated using the internal state itself i.e. UpdateFromBox()
  // EnableCallbacks() sets the callback methods, while
  // DisableCallbacks() sets them to NULL
  void EnableCallbacks();
  void DisableCallbacks();

  vtkKWFrame*        ControlFrame;
  vtkKWLabel*        TranslateLabel;
  vtkKWThumbWheel*   TranslateThumbWheel[3];
  vtkKWLabel*        ScaleLabel;
  vtkKWThumbWheel*   ScaleThumbWheel[3];
  vtkKWLabel*        OrientationLabel;
  vtkKWScale*        OrientationScale[3];

  vtkSMProxy *BoxProxy;
  vtkSMProxy *BoxTransformProxy;
  vtkSMProxy *BoxMatrixProxy;
  vtkSMProxy *BoxPointsProxy;
  vtkSMProxy *BoxNormalsProxy;
  char *BoxProxyName;
  char *BoxTransformProxyName;
  char *BoxMatrixProxyName;
  char *BoxPointsProxyName;
  char *BoxNormalsProxyName;
  vtkSetStringMacro(BoxProxyName);
  vtkSetStringMacro(BoxTransformProxyName);
  vtkSetStringMacro(BoxMatrixProxyName);
  vtkSetStringMacro(BoxPointsProxyName);
  vtkSetStringMacro(BoxNormalsProxyName);
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // For saving the widget into a VTK tcl script.
  // This saves the implicit sphere.  Parts will share this
  // one sphere.
  virtual void SaveInBatchScript(ofstream *file);

  double* GetPositionFromGUI();
  double* GetRotationFromGUI();
  double* GetScaleFromGUI();
  vtkSetVector3Macro(PositionGUI, double);
  vtkSetVector3Macro(RotationGUI, double);
  vtkSetVector3Macro(ScaleGUI,    double);
  double PositionGUI[3];
  double RotationGUI[3];
  double ScaleGUI[3];

  int Initialized;

private:
  vtkPVBoxWidget(const vtkPVBoxWidget&); // Not implemented
  void operator=(const vtkPVBoxWidget&); // Not implemented
};

#endif
