/*=========================================================================

  Module:    vtkKWVolumeMaterialPropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWVolumeMaterialPropertyWidget - widget to control the material property of a volume  (vtkVolumeProperty)
// .SECTION Description

#ifndef __vtkKWVolumeMaterialPropertyWidget_h
#define __vtkKWVolumeMaterialPropertyWidget_h

#include "vtkKWMaterialPropertyWidget.h"

class vtkKWCheckButtonWithLabel;
class vtkKWScalarComponentSelectionWidget;
class vtkVolumeProperty;

class KWWidgets_EXPORT vtkKWVolumeMaterialPropertyWidget : public vtkKWMaterialPropertyWidget
{
public:
  static vtkKWVolumeMaterialPropertyWidget *New();
  vtkTypeRevisionMacro(vtkKWVolumeMaterialPropertyWidget, vtkKWMaterialPropertyWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the volume property to edit with this widget
  virtual void SetVolumeProperty(vtkVolumeProperty *prop);
  vtkGetObjectMacro(VolumeProperty, vtkVolumeProperty);

  // Description:
  // Set/Get the component controlled by the widget
  virtual void SetSelectedComponent(int);
  vtkGetMacro(SelectedComponent, int);
  vtkGetObjectMacro(ComponentSelectionWidget, 
                    vtkKWScalarComponentSelectionWidget);

  // Description:
  // Set/Get the number of components controlled by the widget
  virtual void SetNumberOfComponents(int);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // Allow enable shading per component
  virtual void SetAllowEnableShading(int);
  vtkBooleanMacro(AllowEnableShading, int);
  vtkGetMacro(AllowEnableShading, int);

  // Description:
  // Refresh the interface given the value extracted from the current property.
  virtual void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void EnableShadingCallback(int state);
  virtual void SelectedComponentCallback(int);

protected:
  vtkKWVolumeMaterialPropertyWidget();
  ~vtkKWVolumeMaterialPropertyWidget();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  vtkVolumeProperty *VolumeProperty;

  int SelectedComponent;
  int NumberOfComponents;
  int AllowEnableShading;

  // UI

  vtkKWScalarComponentSelectionWidget *ComponentSelectionWidget;
  vtkKWCheckButtonWithLabel             *EnableShadingCheckButton;

  // Description:
  // Pack
  virtual void Pack();

  // Description:
  // Update the property from the interface values or a preset
  // Return 1 if the property was modified, 0 otherwise
  virtual int UpdatePropertyFromInterface();
  virtual int UpdatePropertyFromPreset(const Preset *preset);

  // Description:
  // Send an event representing the state of the widget
  virtual void SendStateEvent(int event);

  // Description:
  // Return 1 if the controls should be enabled.
  virtual int AreControlsEnabled();

private:
  vtkKWVolumeMaterialPropertyWidget(const vtkKWVolumeMaterialPropertyWidget&);  //Not implemented
  void operator=(const vtkKWVolumeMaterialPropertyWidget&);  //Not implemented
};

#endif
