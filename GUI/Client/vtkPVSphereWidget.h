/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSphereWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSphereWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// vtkPVSphereWidget can be considered as equivalent to the combination of
// vtkPVLineWidget and vtkPVLineSourceWidget.
// Unlike vtkPVLineWidget, vtkPVSphereWidget is never used without the 
// implicit function, hence there was no need to have the distinction here.
// 

#ifndef __vtkPVSphereWidget_h
#define __vtkPVSphereWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;
class vtkPVInputMenu;

class VTK_EXPORT vtkPVSphereWidget : public vtkPV3DWidget
{
public:
  static vtkPVSphereWidget* New();
  vtkTypeRevisionMacro(vtkPVSphereWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Callback that set the center to the middle of the bounds.
  void CenterResetCallback();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  void SetCenter();
  void SetCenter(double,double,double);
  void SetCenter(double c[3]) { this->SetCenter(c[0], c[1], c[2]); }
  void GetCenter(double pts[3]);
  
  void SetRadius();
  void SetRadius(double);
  double GetRadius();

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
  //virtual void AcceptInternal(vtkClientServerID);
  virtual void Accept();
  //ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Provide access to the proxy used by this widget.
  // Sphere == SphereProxy
  //vtkSMProxy* GetProxyByName(const char*);

  // Description:
  // Returns the implicit function proxy.
  // May be,we should use the GetProxyByName interface 
  // but for now...we use this one.
  //virtual vtkSMProxy* GetImplicitFunctionProxy() { return this->ImplicitFunctionProxy; }
  virtual vtkSMProxy* GetProxyByName(const char*);

  // Description:
  // Called when the input changes (before accept).
  virtual void Update();

  // Description:
  // The input from the input menu is used to place the widget.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  // Overloaded to create the ImplicitFunction proxy
  //BTX
  virtual void Create(vtkKWApplication *app);
  //ETX
protected:
  vtkPVSphereWidget();
  ~vtkPVSphereWidget();

  // Description:
  // These methods assume that the Property has been
  // updated before calling them; i.e. Property->UpdateInformation
  // has been invoked.  
  void GetCenterInternal(double pt[3]);
  double GetRadiusInternal();

  void SetCenterInternal(double,double,double);
  void SetCenterInternal(double c[3]) 
    { 
    this->SetCenterInternal(c[0], c[1], c[2]); 
    }
  void SetRadiusInternal(double);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  vtkKWEntry *CenterEntry[3];
  vtkKWEntry *RadiusEntry;
  vtkKWPushButton *CenterResetButton;

  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];

  vtkPVInputMenu* InputMenu;

  vtkSMProxy *ImplicitFunctionProxy;
  char *ImplicitFunctionProxyName;
  vtkSetStringMacro(ImplicitFunctionProxyName);

  // Description:
  // For saving the widget into a VTK tcl script.
  // This saves the implicit sphere.  Parts will share this
  // one sphere.
  virtual void SaveInBatchScript(ofstream *file);

  virtual void ActualPlaceWidget();

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

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
private:
  vtkPVSphereWidget(const vtkPVSphereWidget&); // Not implemented
  void operator=(const vtkPVSphereWidget&); // Not implemented
};

#endif
