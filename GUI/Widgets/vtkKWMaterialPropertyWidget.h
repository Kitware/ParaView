/*=========================================================================

  Module:    vtkKWMaterialPropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMaterialPropertyWidget - widget to control the material property of a volume
// .SECTION Description

#ifndef __vtkKWMaterialPropertyWidget_h
#define __vtkKWMaterialPropertyWidget_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWFrameLabeled;
class vtkKWLabelLabeled;
class vtkKWPushButtonSetLabeled;
class vtkKWPopupButtonLabeled;
class vtkKWPushButton;
class vtkKWScale;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWMaterialPropertyWidget : public vtkKWWidget
{
public:
  vtkTypeRevisionMacro(vtkKWMaterialPropertyWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Display the UI as a popup (default is off). The pushbutton will display
  // a representation of the current properties.
  // This has to be called before Create().
  vtkSetMacro(PopupMode, int);
  vtkGetMacro(PopupMode, int);
  vtkBooleanMacro(PopupMode, int);
  vtkGetObjectMacro(PopupButton, vtkKWPopupButtonLabeled);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Refresh the interface given the value extracted from the current widget.
  virtual void Update();

  // Description:
  // Update the preview according to current settings
  virtual void UpdatePreview();

  // Description:
  // Set/Get the size of the preview, presets and popup preview images
  virtual void SetPreviewSize(int);
  virtual void SetPresetSize(int);
  virtual void SetPopupPreviewSize(int);
  vtkGetMacro(PreviewSize, int);
  vtkGetMacro(PresetSize, int);
  vtkGetMacro(PopupPreviewSize, int);

  // Description:
  // Set/Get the grid opacity in the preview/presets
  virtual void SetGridOpacity(float);
  vtkGetMacro(GridOpacity, float);
  
  // Description:
  // Set/Get the color of the preview/presets.
  vtkGetVector3Macro(MaterialColor, double);
  void SetMaterialColor(double r, double g, double b);
  void SetMaterialColor(double color[3])
    { this->SetMaterialColor(color[0], color[1], color[2]); }

  // Description:
  // Set/Get the event invoked when the property is changed/changing.
  // Defaults to vtkKWEvent::MaterialPropertyChanged/ingEvent, this default
  // is likely to change in subclasses to reflect what kind of property
  // is changed  (vtkKWEvent::VolumeMaterialPropertyChangedEvent for example).
  vtkSetMacro(PropertyChangedEvent, int);
  vtkGetMacro(PropertyChangedEvent, int);
  vtkSetMacro(PropertyChangingEvent, int);
  vtkGetMacro(PropertyChangingEvent, int);

  // Description:
  // Set the command that will be called when the property is changed/changing.
  virtual void SetPropertyChangedCommand(
    vtkKWObject *object, const char *method);
  virtual void SetPropertyChangingCommand(
    vtkKWObject *object, const char *method);
  virtual void InvokePropertyChangedCommand();
  virtual void InvokePropertyChangingCommand();

  // Description:
  // Callbacks for the buttons, scales and presets
  virtual void PropertyChangingCallback();
  virtual void PropertyChangedCallback();
  virtual void PresetMaterialCallback(vtkIdType preset_idx);

  //BTX
  // Description:
  // Preset structure
  class Preset
  {
  public:
    float Ambient;
    float Diffuse;
    float Specular;
    float SpecularPower;
    char *HelpString;

    Preset() { this->HelpString = 0; };
  };
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

  vtkKWMaterialPropertyWidget();
  ~vtkKWMaterialPropertyWidget();
  
  int   PopupMode;
  int   PreviewSize;
  int   PresetSize;
  int   PopupPreviewSize;
  float GridOpacity;

  double MaterialColor[3];

  int   PropertyChangedEvent;
  int   PropertyChangingEvent;

  // Commands

  char  *PropertyChangedCommand;
  char  *PropertyChangingCommand;

  virtual void InvokeCommand(const char *command);

  // Presets

  //BTX
  typedef vtkLinkedList<Preset*> PresetsContainer;
  typedef vtkLinkedListIterator<Preset*> PresetsContainerIterator;
  PresetsContainer *Presets;
  //ETX

  // UI

  vtkKWPopupButtonLabeled   *PopupButton;
  vtkKWFrameLabeled         *MaterialPropertiesFrame;
  vtkKWFrame                *LightingFrame;
  vtkKWScale                *AmbientScale;
  vtkKWScale                *DiffuseScale;
  vtkKWScale                *SpecularScale;
  vtkKWScale                *SpecularPowerScale;
  vtkKWFrame                *PresetsFrame;
  vtkKWLabelLabeled         *PreviewLabel;
  vtkKWPushButtonSetLabeled *PresetPushButtonSet;

  // Create a preview image given material properties
  virtual void CreateImage(unsigned char *data, 
                           float ambient, 
                           float diffuse,
                           float specular, 
                           float specularPower, int size);
  
  // Send an event representing the state of the widget
  virtual void SendStateEvent(int event);

  // Description:
  // Add default presets
  virtual void AddDefaultPresets();

  // Description:
  // Create the presets
  virtual void CreatePresets();

  // Description:
  // Update the popup preview according to current settings
  virtual void UpdatePopupPreview();

  // Description:
  // Update the property from the interface values or a preset
  // Return 1 if the property was modified, 0 otherwise
  virtual int UpdatePropertyFromInterface() = 0;
  virtual int UpdatePropertyFromPreset(const Preset *preset) = 0;

private:
  vtkKWMaterialPropertyWidget(const vtkKWMaterialPropertyWidget&);  //Not implemented
  void operator=(const vtkKWMaterialPropertyWidget&);  //Not implemented
};

#endif
