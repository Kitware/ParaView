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
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVSphereWidget_h
#define __vtkPVSphereWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

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
  vtkPVSphereWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  void SetCenter();
  void SetCenter(double,double,double);
  void SetCenterInternal(double,double,double);
  void SetCenter(double c[3]) { this->SetCenter(c[0], c[1], c[2]); }
  void SetCenterInternal(double c[3]) 
    { 
      this->SetCenterInternal(c[0], c[1], c[2]); 
    }
  void SetRadius();
  void SetRadius(double);
  void SetRadiusInternal(double);

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
  vtkSMProxy* GetProxyByName(const char*);
  
protected:
  vtkPVSphereWidget();
  ~vtkPVSphereWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  void UpdateVTKObject(const char* sourceTclName);

  vtkKWEntry *CenterEntry[3];
  vtkKWEntry *RadiusEntry;
  vtkKWPushButton *CenterResetButton;

  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];

  vtkSMProxy *SphereProxy;
  char *SphereProxyName;
  vtkSetStringMacro(SphereProxyName);
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // For saving the widget into a VTK tcl script.
  // This saves the implicit sphere.  Parts will share this
  // one sphere.
  virtual void SaveInBatchScript(ofstream *file);

private:
  vtkPVSphereWidget(const vtkPVSphereWidget&); // Not implemented
  void operator=(const vtkPVSphereWidget&); // Not implemented
};

#endif
