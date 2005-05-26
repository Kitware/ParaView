/*=========================================================================

  Module:    vtkKWVolumeMaterialPropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWVolumeMaterialPropertyWidget - widget to control the material property of a volume
// .SECTION Description

#ifndef __vtkKWVolumeMaterialPropertyWidget_h
#define __vtkKWVolumeMaterialPropertyWidget_h

#include "vtkKWMaterialPropertyWidget.h"

class vtkKWApplication;
class vtkKWCheckButtonLabeled;
class vtkKWScalarComponentSelectionWidget;
class vtkVolumeProperty;

class KWWIDGETS_EXPORT vtkKWVolumeMaterialPropertyWidget : public vtkKWMaterialPropertyWidget
{
public:
  static vtkKWVolumeMaterialPropertyWidget *New();
  vtkTypeRevisionMacro(vtkKWVolumeMaterialPropertyWidget, vtkKWMaterialPropertyWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Set/get the volume property observed by this widget
//BTX
  virtual void SetVolumeProperty(vtkVolumeProperty *prop);
  vtkGetObjectMacro(VolumeProperty, vtkVolumeProperty);
//ETX

  // Description:
  // Set/get the component controlled by the widget
  virtual void SetSelectedComponent(int);
  vtkGetMacro(SelectedComponent, int);
  vtkGetObjectMacro(ComponentSelectionWidget, 
                    vtkKWScalarComponentSelectionWidget);

  // Description:
  // Set/get the number of components controlled by the widget
  virtual void SetNumberOfComponents(int);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // Allow enable shading per component
  virtual void SetAllowEnableShading(int);
  vtkBooleanMacro(AllowEnableShading, int);
  vtkGetMacro(AllowEnableShading, int);

  // Description:
  // Refresh the interface given the value extracted from the current widget.
  virtual void Update();

  // Description:
  // Callbacks for the buttons, scales and presets
  virtual void EnableShadingCallback();
  virtual void SelectedComponentCallback(int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:

  vtkKWVolumeMaterialPropertyWidget();
  ~vtkKWVolumeMaterialPropertyWidget();

  vtkVolumeProperty *VolumeProperty;

  int SelectedComponent;
  int NumberOfComponents;
  int AllowEnableShading;

  // UI

  vtkKWScalarComponentSelectionWidget *ComponentSelectionWidget;
  vtkKWCheckButtonLabeled             *EnableShadingCheckButton;

  // Pack
  virtual void Pack();

  // Description:
  // Update the property from the interface values or a preset
  // Return 1 if the property was modified, 0 otherwise
  virtual int UpdatePropertyFromInterface();
  virtual int UpdatePropertyFromPreset(const Preset *preset);

  // Send an event representing the state of the widget
  virtual void SendStateEvent(int event);

private:
  vtkKWVolumeMaterialPropertyWidget(const vtkKWVolumeMaterialPropertyWidget&);  //Not implemented
  void operator=(const vtkKWVolumeMaterialPropertyWidget&);  //Not implemented
};

#endif
