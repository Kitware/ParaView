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

#include "vtkKWCompositeWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWFrameWithLabel;
class vtkKWLabelWithLabel;
class vtkKWPushButtonSetWithLabel;
class vtkKWPopupButtonWithLabel;
class vtkKWPushButton;
class vtkKWScaleWithEntry;
class vtkKWMaterialPropertyWidgetInternals;

class KWWidgets_EXPORT vtkKWMaterialPropertyWidget : public vtkKWCompositeWidget
{
public:
  vtkTypeRevisionMacro(vtkKWMaterialPropertyWidget, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Display the UI as a popup (default is off). The pushbutton will display
  // a representation of the current properties.
  // This has to be called before Create().
  vtkSetMacro(PopupMode, int);
  vtkGetMacro(PopupMode, int);
  vtkBooleanMacro(PopupMode, int);
  vtkGetObjectMacro(PopupButton, vtkKWPopupButtonWithLabel);
  
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
  virtual void SetGridOpacity(double);
  vtkGetMacro(GridOpacity, double);
  
  // Description:
  // Set/Get the color of the preview/presets.
  vtkGetVector3Macro(MaterialColor, double);
  void SetMaterialColor(double r, double g, double b);
  void SetMaterialColor(double color[3])
    { this->SetMaterialColor(color[0], color[1], color[2]); }

  // Description:
  // Set/Get the lighting parameters visibility.
  // If set to Off, none of the ambient, diffuse, specular (etc.) scales
  // will be displayed.
  virtual void SetLightingParametersVisibility(int);
  vtkBooleanMacro(LightingParametersVisibility, int);
  vtkGetMacro(LightingParametersVisibility, int);

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
  // Specifies commands to associate with the widget. 
  // 'PropertyChangedCommand' is invoked when the property has
  // changed (i.e. at the end of the user interaction).
  // 'PropertyChangingCommand' is invoked when the selected color is
  // changing (i.e. during the user interaction).
  // The need for a '...ChangedCommand' and '...ChangingCommand' can be
  // explained as follows: the former can be used to be notified about any
  // changes made to this widget *after* the corresponding user interaction has
  // been performed (say, after releasing the mouse button that was dragging
  // a slider, or after clicking on a checkbutton). The later can be set
  // *additionally* to be notified about the intermediate changes that
  // occur *during* the corresponding user interaction (say, *while* dragging
  // a slider). While setting '...ChangedCommand' is enough to be notified
  // about any changes, setting '...ChangingCommand' is an application-specific
  // choice that is likely to depend on how fast you want (or can) answer to
  // rapid changes occuring during a user interaction, if any.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetPropertyChangedCommand(
    vtkObject *object, const char *method);
  virtual void SetPropertyChangingCommand(
    vtkObject *object, const char *method);

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
  virtual void PropertyChangingCallback(double value);
  virtual void PropertyChangedCallback(double value);
  virtual void PresetMaterialCallback(int preset_idx);

protected:
  vtkKWMaterialPropertyWidget();
  ~vtkKWMaterialPropertyWidget();
  
  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  int   PopupMode;
  int   PreviewSize;
  int   PresetSize;
  int   PopupPreviewSize;
  double GridOpacity;
  int LightingParametersVisibility;

  double MaterialColor[3];

  // Description:
  // Events
  int   PropertyChangedEvent;
  int   PropertyChangingEvent;

  // Description:
  // Commands
  char  *PropertyChangedCommand;
  char  *PropertyChangingCommand;

  virtual void InvokePropertyChangedCommand();
  virtual void InvokePropertyChangingCommand();

  // Presets

  //BTX
  class Preset
  {
  public:
    double Ambient;
    double Diffuse;
    double Specular;
    double SpecularPower;
    char *HelpString;

    Preset() { this->HelpString = 0; };
  };

  // PIMPL Encapsulation for STL containers

  vtkKWMaterialPropertyWidgetInternals *Internals;
  friend class vtkKWMaterialPropertyWidgetInternals;
  //ETX

  // UI

  vtkKWPopupButtonWithLabel   *PopupButton;
  vtkKWFrameWithLabel         *MaterialPropertiesFrame;
  vtkKWFrame                  *ControlFrame;
  vtkKWFrame                  *LightingFrame;
  vtkKWScaleWithEntry         *AmbientScale;
  vtkKWScaleWithEntry         *DiffuseScale;
  vtkKWScaleWithEntry         *SpecularScale;
  vtkKWScaleWithEntry         *SpecularPowerScale;
  vtkKWFrame                  *PresetsFrame;
  vtkKWLabelWithLabel         *PreviewLabel;
  vtkKWPushButtonSetWithLabel *PresetPushButtonSet;

  // Description:
  // Pack
  virtual void Pack();

  // Description:
  // Create a preview image given some material properties
  virtual void CreateImage(unsigned char *data, 
                           double ambient, 
                           double diffuse,
                           double specular, 
                           double specular_power, 
                           int size);
  
  // Description:
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

  // Description:
  // Update the scales from a preset
  virtual int UpdateScalesFromPreset(const Preset *preset);
  virtual void UpdateScales(double ambient, 
                            double diffuse,
                            double specular, 
                            double specular_power);

  // Description:
  // Return 1 if the controls should be enabled.
  virtual int AreControlsEnabled() { return 1; };

private:
  vtkKWMaterialPropertyWidget(const vtkKWMaterialPropertyWidget&);  //Not implemented
  void operator=(const vtkKWMaterialPropertyWidget&);  //Not implemented
};

#endif
