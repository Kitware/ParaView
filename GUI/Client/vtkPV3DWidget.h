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

class vtkPV3DWidgetObserver;
class vtkKWFrame;
class vtkSMProxy;
class vtkSM3DWidgetProxy;

class VTK_EXPORT vtkPV3DWidget : public vtkPVObjectWidget
{
public:
  vtkTypeRevisionMacro(vtkPV3DWidget, vtkPVObjectWidget);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the 3DWidget. 
  // Creates a SM3DWidgetProxy. The actual proxy XML name is
  // determined using WidgetProxyXMLName which is set by derrived
  // clases of this class.
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

  //BTX
  // Description:
  // Move widget state to VTK object or back.
  virtual void Accept();
  virtual void ResetInternal();

  // Description:
  // Initialize the newly created widget.
  virtual void Initialize();
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
  // Provide access to the proxy used by this widget.
  virtual vtkSMProxy* GetProxyByName(const char*) { return NULL; }
//BTX
  vtkGetObjectMacro(WidgetProxy,vtkSM3DWidgetProxy);
  vtkGetStringMacro(WidgetProxyName);
  vtkGetStringMacro(WidgetProxyXMLName);
//ETX

protected:
  vtkPV3DWidget();
  ~vtkPV3DWidget();
  
  virtual void PlaceWidget(double bds[6]);
  
  // Description:
  // Initialize observers on the SM3DWidgetProxy.
  void InitializeObservers(vtkSM3DWidgetProxy* widgetproxy); 

  void Render();

  vtkPV3DWidgetObserver* Observer;
  vtkSM3DWidgetProxy *WidgetProxy;
  char *WidgetProxyName;
  char *WidgetProxyXMLName; //The name of the SM3DWidgetProxy to create
  vtkSetStringMacro(WidgetProxyName);
  vtkSetStringMacro(WidgetProxyXMLName);
  
  // Description:
  // Set label of the frame
  void SetFrameLabel(const char* label);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*) = 0;
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);

  friend class vtkPV3DWidgetObserver;
//ETX

  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
  
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);

  vtkKWFrame*        Frame;
  vtkKWLabeledFrame* LabeledFrame;
  vtkKWCheckButton* Visibility;
  int ValueChanged;
  int Placed;
  int Visible;
  int UseLabel;

private:  
  vtkPV3DWidget(const vtkPV3DWidget&); // Not implemented
  void operator=(const vtkPV3DWidget&); // Not implemented
};

#endif
