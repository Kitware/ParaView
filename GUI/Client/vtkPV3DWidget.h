/*=========================================================================

  Program:   ParaView
  Module:    vtkPV3DWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPV3DWidget - superclass of 3D Widgets
// .SECTION Description

// Todo:
// Cleanup GUI:
//       * Visibility
//       * Resolution
//       *

#ifndef __vtkPV3DWidget_h
#define __vtkPV3DWidget_h

#include "vtkPVObjectWidget.h"

class vtkKWCheckButton;
class vtkKWLabeledFrame;
class vtk3DWidget;
class vtkPV3DWidgetObserver;
class vtkKWFrame;

class vtkRM3DWidget;

class VTK_EXPORT vtkPV3DWidget : public vtkPVObjectWidget
{
public:
  vtkTypeRevisionMacro(vtkPV3DWidget, vtkPVObjectWidget);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Set the widget visibility.
  void SetVisibility();
  virtual void SetVisibility(int val);
  void SetVisibilityNoTrace(int val);
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Set modified to 1 when value has changed.
  void SetValueChanged();

  // Description:
  // This method is called when the source that contains this widget
  // is selected. 
  virtual void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected. 
  virtual void Deselect();

  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void PlaceWidget();

  // Description:
  // This method does the actual placing. If the subclass is doing
  // something fancy, it should overwrite it.
  virtual void ActualPlaceWidget();

  // Description:
  // Determines whether there is a label-border around the widget
  // ui.
  vtkSetMacro(UseLabel, int);
  vtkGetMacro(UseLabel, int);

  // Description:
  // Internal method used only to get the widget pointer.
  void InitializeObservers(vtk3DWidget* widget3D); 

  //BTX
  // Description:
  // Move widget state to VTK object or back.
  virtual void AcceptInternal(vtkClientServerID);
  virtual void ResetInternal();
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPV3DWidget();
  ~vtkPV3DWidget();

  void Render();

  vtkPV3DWidgetObserver* Observer;

  // Description:
  // Set label of the frame
  void SetFrameLabel(const char* label);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*) = 0;

  // Description:
  // Set the 3D widget we are observing.
  void SetObservable3DWidget(vtk3DWidget*);

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);

  friend class vtkPV3DWidgetObserver;
//ETX

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
  
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);

  vtkKWFrame*        Frame;
  vtkKWLabeledFrame* LabeledFrame;
  vtkKWCheckButton* Visibility;
  vtkRM3DWidget* RM3DWidget;
  int ValueChanged;
  int Placed;
  int Visible;
  int UseLabel;

  vtk3DWidget* Widget3D;  

private:  
  vtkPV3DWidget(const vtkPV3DWidget&); // Not implemented
  void operator=(const vtkPV3DWidget&); // Not implemented
};

#endif
