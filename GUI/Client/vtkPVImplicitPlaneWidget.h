/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImplicitPlaneWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not decide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVImplicitPlaneWidget_h
#define __vtkPVImplicitPlaneWidget_h

#include "vtkPV3DWidget.h"

class vtkPVAnimationInterfaceEntry;
class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;
class vtkPVInputMenu;

class VTK_EXPORT vtkPVImplicitPlaneWidget : public vtkPV3DWidget
{
public:
  static vtkPVImplicitPlaneWidget* New();
  vtkTypeRevisionMacro(vtkPVImplicitPlaneWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Callback that set the center to the middle of the bounds.
  void CenterResetCallback();

  // Descript:
  // Callbacks to set the normal.
  void NormalCameraCallback();
  void NormalXCallback();
  void NormalYCallback();
  void NormalZCallback();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);


  // Description:
  // Center of the plane.
  void SetCenter();
  virtual void SetCenter(double,double,double);
  virtual void SetCenter(double f[3]) { this->SetCenter(f[0], f[1], f[2]); }
  void GetCenter(double pts[3]);

  // Description:
  // The normal to the plane.
  void SetNormal();
  virtual void SetNormal(double,double,double);
  virtual void SetNormal(double f[3]) { this->SetNormal(f[0], f[1], f[2]); }
  void GetNormal(double pts[3]);
  
  // Description:
  // The input from the input menu is used to place the widget.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  // For saving the widget into a VTK tcl script.
  // One plane object is create for all parts.
  virtual void SaveInBatchScript(ofstream *file);

  // Description: 
  // Called when the input chages (before accept).
  virtual void Update();

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
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
  // Plane == PlaneProxy
  virtual vtkSMProxy* GetProxyByName(const char*);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(
    vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Called when menu item (above) is selected.  Neede for tracing.
  // Would not be necessary if menus traced invocations.
  virtual void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Calls UpdateVTKObjects on the plane proxy.
  virtual void UpdateVTKObjects();

  // Description:
  // Resets the animation entries (start and end) to values obtained
  // from the range domain of the Offset property to the plane proxy
  virtual void ResetAnimationRange(vtkPVAnimationInterfaceEntry* ai);

  // Description:
  // Overloaded to create the ImplicitFunctionProxy
  // BTX
  virtual void Create(vtkKWApplication *app);
  //ETX

  // Description:
  // Updates the Offset property of the plane proxy 
  // and calls ModifiedCallback.
  void UpdateOffsetRange();

protected:
  vtkPVImplicitPlaneWidget();
  ~vtkPVImplicitPlaneWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  virtual void SetCenterInternal(double,double,double);
  virtual void SetNormalInternal(double,double,double);

  // Description:
  // These methods assume that UpdateInformation() has been
  // called on the WidgetProxy()
  void GetCenterInternal(double pts[3]);
  void GetNormalInternal(double pts[3]);
  
  vtkPVInputMenu *InputMenu;

  vtkKWEntry *CenterEntry[3];
  vtkKWPushButton *CenterResetButton;

  vtkKWEntry *NormalEntry[3];
  vtkKWEntry *OffsetEntry;

  vtkKWWidget *NormalButtonFrame;
  vtkKWPushButton *NormalCameraButton;
  vtkKWPushButton *NormalXButton;
  vtkKWPushButton *NormalYButton;
  vtkKWPushButton *NormalZButton;
  vtkKWLabel* Labels[2];
  vtkKWLabel* OffsetLabel;
  vtkKWLabel* CoordinateLabel[3];

  vtkSMProxy *ImplicitFunctionProxy;
  char *ImplicitFunctionProxyName;
  vtkSetStringMacro(ImplicitFunctionProxyName);

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
  vtkPVImplicitPlaneWidget(const vtkPVImplicitPlaneWidget&); // Not implemented
  void operator=(const vtkPVImplicitPlaneWidget&); // Not implemented
};

#endif
