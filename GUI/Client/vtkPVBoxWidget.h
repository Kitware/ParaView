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
class vtkXMProxy;
class vtkPVInputMenu;

class VTK_EXPORT vtkPVBoxWidget : public vtkPV3DWidget
{
public:
  static vtkPVBoxWidget* New();
  vtkTypeRevisionMacro(vtkPVBoxWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
  //virtual void AcceptInternal(vtkClientServerID);
  //ETX
  virtual void Accept();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Callbacks to update the values from the GUI.
  // These methods do nothing is this->ValueChanged is not set
  void SetTranslate();
  void SetOrientation();
  void SetScale();

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
  // Box == BoxProxy
  vtkSMProxy* GetProxyByName(const char* name);
 
  // Description:
  // The input from the input menu is used to place the widget.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  // Called when the input changes (before accept).
  virtual void Update();

  // Description:
  // Place the widget
  virtual void Initialize();

  // Description:
  // Register the animatable proxies and make them avaiblable for animation.
  // Called by vtkPVSelectWidget when the widget is selected.
  virtual void EnableAnimation(){ this->RegisterAnimateableProxies();} ;

  // Description:
  // Unregister animatable proxies so that they are not available for
  // animation. Called by vtkPVSelectWidget when this widget is deselected.
  virtual void DisableAnimation() { this->UnregisterAnimateableProxies();} ;

  // Description:
  // Overloaded to create the ImplicitFunction proxy
  //BTX
  virtual void Create(vtkKWApplication *app);
  //ETX
protected:
  vtkPVBoxWidget();
  ~vtkPVBoxWidget();

  // Description:
  // PlaceWidget is overloaded since, this class has to position the
  // bounds on the BoxProxy(vtkBox) as well.
  virtual void PlaceWidget(double bds[6]);
  virtual void PlaceWidget() { this->Superclass::PlaceWidget(); }
  
  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  // Description:
  // Get iVar values from vtkSMBoxWidgetProxy object and update the GUI.
  void UpdateFromBox();

  vtkKWFrame*        ControlFrame;
  vtkKWLabel*        TranslateLabel;
  vtkKWThumbWheel*   TranslateThumbWheel[3];
  vtkKWLabel*        ScaleLabel;
  vtkKWThumbWheel*   ScaleThumbWheel[3];
  vtkKWLabel*        OrientationLabel;
  vtkKWScale*        OrientationScale[3];

  vtkPVInputMenu*   InputMenu;

  vtkSMProxy *BoxProxy; //The Implicit function proxy
  vtkSMProxy *BoxTransformProxy;

  int ReadXMLAttributes(vtkPVXMLElement* element,
    vtkPVXMLPackageParser* parser);

  // Description:
  // For saving the widget into a VTK tcl script.
  // This saves the implicit sphere.  Parts will share this
  // one sphere.
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  virtual vtkPVWidget* ClonePrototypeInternal(
    vtkPVSource* pvSource,
    vtkArrayMap<vtkPVWidget*, 
    vtkPVWidget*>* map);
  //ETX


  // Description:
  // Methods to obtain the position/rotation/scale values from
  // the vtkSMBoxWidgetProxy object. These methods do not call
  // UpdateInformation(), hence, it is expeceted that the information
  // properties have been updated before calling these methods.
  void GetPositionInternal(double position[3]);
  void GetRotationInternal(double rotation[3]);
  void GetScaleInternal(double scale[3]);
  
  double* GetPositionFromGUI();
  double* GetRotationFromGUI();
  double* GetScaleFromGUI();
  vtkSetVector3Macro(PositionGUI, double);
  vtkSetVector3Macro(RotationGUI, double);
  vtkSetVector3Macro(ScaleGUI,    double);
  double PositionGUI[3];
  double RotationGUI[3];
  double ScaleGUI[3];

  void SetupPropertyObservers();
  void UnsetPropertyObservers();

  void RegisterAnimateableProxies();
  void UnregisterAnimateableProxies();
private:
  vtkPVBoxWidget(const vtkPVBoxWidget&); // Not implemented
  void operator=(const vtkPVBoxWidget&); // Not implemented
};

#endif
