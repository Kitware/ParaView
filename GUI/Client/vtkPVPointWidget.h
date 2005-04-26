/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPointWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPointWidget_h
#define __vtkPVPointWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

class VTK_EXPORT vtkPVPointWidget : public vtkPV3DWidget
{
public:
  static vtkPVPointWidget* New();
  vtkTypeRevisionMacro(vtkPVPointWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Callback that set the center to the middle of the bounds.
  void PositionResetCallback();

  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

  void SetPosition();
  void SetPosition(double,double,double);
  void GetPosition(double pt[3]);

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  // Description:
  // Places the widget
  virtual void Initialize();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
  virtual void Accept();
  //ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // Display hint about picking using the p key.
  void SetVisibility(int v);  

  // Description:
  // Overridden to set up control dependencies among properties.
  virtual void Create(vtkKWApplication* app);

protected:
  vtkPVPointWidget();
  ~vtkPVPointWidget();

  void SetPositionInternal(double,double,double);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  // Description:
  // This method assumes that WidgetProxy->UpdateInformation() has been invoked before calling
  // this method.
  void GetPositionInternal(double pt[3]);

  vtkKWEntry *PositionEntry[3];
  vtkKWPushButton *PositionResetButton;

  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPointWidget(const vtkPVPointWidget&); // Not implemented
  void operator=(const vtkPVPointWidget&); // Not implemented
};

#endif
