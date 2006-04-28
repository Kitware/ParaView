/*=========================================================================

  Module:    vtkKWMaterialPropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMaterialPropertyWidget.h"

#include "vtkKWCheckButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkKWPopupButtonWithLabel.h"
#include "vtkKWPushButtonSetWithLabel.h"
#include "vtkKWPopupButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/list>

//----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkKWMaterialPropertyWidget, "1.27");

//----------------------------------------------------------------------------
class vtkKWMaterialPropertyWidgetInternals
{
public:

  typedef vtksys_stl::list<vtkKWMaterialPropertyWidget::Preset*> PresetsContainer;
  typedef vtksys_stl::list<vtkKWMaterialPropertyWidget::Preset*>::iterator PresetsContainerIterator;

  PresetsContainer Presets;
};

//----------------------------------------------------------------------------
vtkKWMaterialPropertyWidget::vtkKWMaterialPropertyWidget()
{
  this->PreviewSize      = 40;
  this->PresetSize       = 40;
  this->PopupPreviewSize = 24;
  this->GridOpacity      = 0.3;
  this->LightingParametersVisibility  = 1;

  this->MaterialColor[0] = 1.0;
  this->MaterialColor[1] = 1.0;
  this->MaterialColor[2] = 1.0;
  
  this->PropertyChangedEvent  = vtkKWEvent::MaterialPropertyChangedEvent;
  this->PropertyChangingEvent = vtkKWEvent::MaterialPropertyChangingEvent;

  this->PropertyChangedCommand  = NULL;
  this->PropertyChangingCommand = NULL;

  // Presets

  this->Internals = new vtkKWMaterialPropertyWidgetInternals;
  this->AddDefaultPresets();

  // UI

  this->PopupMode = 0;

  this->PopupButton = NULL;

  this->MaterialPropertiesFrame = vtkKWFrameWithLabel::New();

  this->ControlFrame = vtkKWFrame::New();

  this->LightingFrame = vtkKWFrame::New();

  this->AmbientScale  = vtkKWScaleWithEntry::New();

  this->DiffuseScale  = vtkKWScaleWithEntry::New();

  this->SpecularScale = vtkKWScaleWithEntry::New();

  this->SpecularPowerScale = vtkKWScaleWithEntry::New();

  this->PresetsFrame = vtkKWFrame::New();

  this->PreviewLabel = vtkKWLabelWithLabel::New();

  this->PresetPushButtonSet = vtkKWPushButtonSetWithLabel::New();
}

//----------------------------------------------------------------------------
vtkKWMaterialPropertyWidget::~vtkKWMaterialPropertyWidget()
{
  // Commands

  if (this->PropertyChangedCommand)
    {
    delete [] this->PropertyChangedCommand;
    this->PropertyChangedCommand = NULL;
    }

  if (this->PropertyChangingCommand)
    {
    delete [] this->PropertyChangingCommand;
    this->PropertyChangingCommand = NULL;
    }

  // Delete all presets

  if (this->Internals)
    {
    vtkKWMaterialPropertyWidgetInternals::PresetsContainerIterator it = 
      this->Internals->Presets.begin();
    vtkKWMaterialPropertyWidgetInternals::PresetsContainerIterator end = 
      this->Internals->Presets.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        if ((*it)->HelpString)
          {
          delete [] (*it)->HelpString;
          }
        delete (*it);
        }
      }
    delete this->Internals;
    }

  // Delete UI

  if (this->PopupButton)
    {
    this->PopupButton->Delete();
    this->PopupButton = NULL;
    }

  if (this->MaterialPropertiesFrame)
    {
    this->MaterialPropertiesFrame->Delete();
    this->MaterialPropertiesFrame = NULL;
    }

  if (this->ControlFrame)
    {
    this->ControlFrame->Delete();
    this->ControlFrame = NULL;
    }

  if (this->LightingFrame)
    {
    this->LightingFrame->Delete();
    this->LightingFrame = NULL;
    }

  if (this->AmbientScale)
    {
    this->AmbientScale->Delete();
    this->AmbientScale = NULL;
    }

  if (this->DiffuseScale)
    {
    this->DiffuseScale->Delete();
    this->DiffuseScale = NULL;
    }

  if (this->SpecularScale)
    {
    this->SpecularScale->Delete();
    this->SpecularScale = NULL;
    }

  if (this->SpecularPowerScale)
    {
    this->SpecularPowerScale->Delete();
    this->SpecularPowerScale = NULL;
    }

  if (this->PresetsFrame)
    {
    this->PresetsFrame->Delete();
    this->PresetsFrame = NULL;
    }

  if (this->PreviewLabel)
    {
    this->PreviewLabel->Delete();
    this->PreviewLabel = NULL;
    }

  if (this->PresetPushButtonSet)
    {
    this->PresetPushButtonSet->Delete();
    this->PresetPushButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::AddDefaultPresets()
{
  if (!this->Internals)
    {
    return;
    }

  vtkKWMaterialPropertyWidget::Preset *preset;

  // Presets : Unshaded

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 1.0;
  preset->Diffuse = 0.0;
  preset->Specular = 0.0;
  preset->SpecularPower = 1.0;
  preset->HelpString = vtksys::SystemTools::DuplicateString(
    ks_("Material Preset|Full ambient eliminating all directional shading."));
  this->Internals->Presets.push_back(preset);

  // Presets : Dull

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 0.2;
  preset->Diffuse = 1.0;
  preset->Specular = 0.0;
  preset->SpecularPower = 1.0;
  preset->HelpString = vtksys::SystemTools::DuplicateString(
    ks_("Material Preset|Dull material properties (no specular lighting)"));
  this->Internals->Presets.push_back(preset);
  
  // Presets : Smooth

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 0.1;
  preset->Diffuse = 0.9;
  preset->Specular = 0.2;
  preset->SpecularPower = 10.0;
  preset->HelpString = vtksys::SystemTools::DuplicateString(
    ks_("Material Preset|Smooth material properties (moderate specular lighting"));
  this->Internals->Presets.push_back(preset);
  
  // Presets : Shiny

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 0.1;
  preset->Diffuse = 0.6;
  preset->Specular = 0.5;
  preset->SpecularPower = 40.0;
  preset->HelpString = vtksys::SystemTools::DuplicateString(
    ks_("Material Preset|Shiny material properties (high specular lighting)"));
  this->Internals->Presets.push_back(preset);
} 

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  vtkKWFrame *frame;
  
  // --------------------------------------------------------------
  // If in popup mode, create the popup button

  if (this->PopupMode)
    {
    if (!this->PopupButton)
      {
      this->PopupButton = vtkKWPopupButtonWithLabel::New();
      }
    
    this->PopupButton->SetParent(this);
    this->PopupButton->Create();
    this->PopupButton->GetLabel()->SetText(
      ks_("Material Property Editor|Material:"));
    this->PopupButton->GetWidget()->SetText("");
    this->PopupButton->GetWidget()->SetPopupTitle(
      ks_("Material Property Editor|Material Properties"));

    this->Script("pack %s -side left -anchor w -fill x",
                 this->PopupButton->GetWidgetName());
    }

  // --------------------------------------------------------------
  // Material frame

  if (this->PopupMode)
    {
    this->MaterialPropertiesFrame->AllowFrameToCollapseOff();
    this->MaterialPropertiesFrame->SetParent(
      this->PopupButton->GetWidget()->GetPopupFrame());
    }
  else
    {
    this->MaterialPropertiesFrame->AllowFrameToCollapseOn();
    this->MaterialPropertiesFrame->SetParent(this);
    }
  this->MaterialPropertiesFrame->Create();
  this->MaterialPropertiesFrame->SetLabelText(
    ks_("Material Property Editor|Material Properties"));

  this->Script("pack %s -padx 0 -pady 0 -fill x -expand yes -anchor w",
               this->MaterialPropertiesFrame->GetWidgetName());
  
  frame = this->MaterialPropertiesFrame->GetFrame();

  // --------------------------------------------------------------
  // Control frame

  this->ControlFrame->SetParent(frame);
  this->ControlFrame->Create();

  // --------------------------------------------------------------
  // Lighting frame

  this->LightingFrame->SetParent(frame);
  this->LightingFrame->Create();

  // --------------------------------------------------------------
  // Ambient

  int entry_width = 5;
  int label_width = 12;
  int row = 0;

  this->AmbientScale->SetParent(this->LightingFrame);
  this->AmbientScale->Create();
  this->AmbientScale->SetCommand(this, "PropertyChangingCallback");
  this->AmbientScale->SetEndCommand(this, "PropertyChangedCallback");
  this->AmbientScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->AmbientScale->SetEntryWidth(entry_width);
  this->AmbientScale->SetLabelText(ks_("Material Property Editor|Ambient:"));
  this->AmbientScale->SetLabelWidth(label_width);
  this->AmbientScale->SetRange(0, 100);
  this->AmbientScale->SetBalloonHelpString(
    k_("Set the ambient coefficient within the range [0,100] for lighting"));
  
  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->AmbientScale->GetWidgetName(), row++);
  
  this->Script("grid columnconfigure %s 0 -weight 1",
               this->AmbientScale->GetParent()->GetWidgetName(), row++);
  
  // --------------------------------------------------------------
  // Diffuse

  this->DiffuseScale->SetParent(this->LightingFrame);
  this->DiffuseScale->Create();
  this->DiffuseScale->SetCommand(this, "PropertyChangingCallback");
  this->DiffuseScale->SetEndCommand(this, "PropertyChangedCallback");
  this->DiffuseScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->DiffuseScale->SetEntryWidth(entry_width);
  this->DiffuseScale->SetLabelText(ks_("Material Property Editor|Diffuse:"));
  this->DiffuseScale->SetLabelWidth(label_width);
  this->DiffuseScale->SetRange(0, 100);
  this->DiffuseScale->SetBalloonHelpString(
    k_("Set the diffuse coefficient within the range [0,100] for lighting"));
  
  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->DiffuseScale->GetWidgetName(), row++);
  
  // --------------------------------------------------------------
  // Specular

  this->SpecularScale->SetParent(this->LightingFrame);
  this->SpecularScale->Create();
  this->SpecularScale->SetCommand(this, "PropertyChangingCallback");
  this->SpecularScale->SetEndCommand(this, "PropertyChangedCallback");
  this->SpecularScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->SpecularScale->SetEntryWidth(entry_width);
  this->SpecularScale->SetLabelText(
    ks_("Material Property Editor|Specular:"));
  this->SpecularScale->SetLabelWidth(label_width);
  this->SpecularScale->SetRange(0, 100);
  this->SpecularScale->SetBalloonHelpString(
    k_("Set the specular coefficient within the range [0,100] for lighting"));

  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->SpecularScale->GetWidgetName(), row++);
 
  // --------------------------------------------------------------
  // Specular power

  this->SpecularPowerScale->SetParent(this->LightingFrame);
  this->SpecularPowerScale->Create();
  this->SpecularPowerScale->SetCommand(this, "PropertyChangingCallback");
  this->SpecularPowerScale->SetEndCommand(this, "PropertyChangedCallback");
  this->SpecularPowerScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->SpecularPowerScale->SetEntryWidth(entry_width);
  this->SpecularPowerScale->SetLabelText(
    ks_("Material Property Editor|Power:"));
  this->SpecularPowerScale->SetLabelWidth(label_width);
  this->SpecularPowerScale->SetRange(1, 50);
  this->SpecularPowerScale->SetBalloonHelpString(
    k_("Set the specular power within the range [0,50] for lighting"));
  
  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->SpecularPowerScale->GetWidgetName(), row++);
 
  // --------------------------------------------------------------
  // Presets + Preview frame

  this->PresetsFrame->SetParent(frame);
  this->PresetsFrame->Create();
  
  // --------------------------------------------------------------
  // Preview

  this->PreviewLabel->SetParent(this->PresetsFrame);
  this->PreviewLabel->SetLabelPositionToTop();
  this->PreviewLabel->ExpandWidgetOff();
  this->PreviewLabel->Create();
  this->PreviewLabel->GetLabel()->SetText(
    ks_("Material Property Editor|Preview:"));
  
  this->Script("pack %s -side left -padx 2 -pady 2 -anchor nw",
               this->PreviewLabel->GetWidgetName());
  
  // --------------------------------------------------------------
  // Presets

  this->PresetPushButtonSet->SetParent(this->PresetsFrame);
  this->PresetPushButtonSet->SetLabelPositionToTop();
  this->PresetPushButtonSet->GetLabel()->SetText(
    ks_("Material Property Editor|Presets:"));
  this->PresetPushButtonSet->Create();
  this->PresetPushButtonSet->ExpandWidgetOff();

  this->Script(
    "pack %s -side right -padx 2 -pady 2 -anchor nw",
    this->PresetPushButtonSet->GetWidgetName());

  vtkKWPushButtonSet *pbs = this->PresetPushButtonSet->GetWidget();
  pbs->PackHorizontallyOn();

  this->CreatePresets();

  // Set some default values that are more pleasing the black body

  this->UpdateScales(0.1 * 100, 0.9 * 100, 0.2 * 100, 10.0 * 100);

  // Pack

  this->Pack();

  // Update according to the current view/widget

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->MaterialPropertiesFrame)
    {
    this->MaterialPropertiesFrame->GetFrame()->UnpackChildren();
    }

  if (this->ControlFrame)
    {
    this->Script("pack %s -padx 0 -pady 0 -fill x -expand yes -anchor w",
                 this->ControlFrame->GetWidgetName());
    }
  
  if (this->LightingFrame && this->LightingParametersVisibility)
    {
    this->Script("pack %s -padx 0 -pady 0 -fill x -expand yes -anchor w",
                 this->LightingFrame->GetWidgetName());
    }

  if (this->PresetsFrame)
    {
    this->Script("pack %s -anchor w -fill x -expand y",
                 this->PresetsFrame->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::Update()
{
  // Update enable state
  
  this->UpdateEnableState();

  if (!this->IsCreated())
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetLightingParametersVisibility(int arg)
{
  if (this->LightingParametersVisibility == arg)
    {
    return;
    }

  this->LightingParametersVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPreviewSize(int v)
{
  if (this->PreviewSize == v || v < 8)
    {
    return;
    }

  this->PreviewSize = v;
  this->Modified();

  this->UpdatePreview();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetGridOpacity(double v)
{
  if (this->GridOpacity == v || v < 0.0 || v > 1.0)
    {
    return;
    }

  this->GridOpacity = v;
  this->Modified();

  this->UpdatePreview();
  this->CreatePresets();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetMaterialColor(double r, double g, double b)
{
  if (this->MaterialColor[0] == r &&
      this->MaterialColor[1] == g &&
      this->MaterialColor[2] == b)
    {
    return;
    }

  this->MaterialColor[0] = r;
  this->MaterialColor[1] = g;
  this->MaterialColor[2] = b;

  this->UpdatePreview();
  this->CreatePresets();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::UpdateScales(double ambient, 
                                               double diffuse,
                                               double specular, 
                                               double specular_power)
{
  // Ambient

  if (this->AmbientScale && this->AmbientScale->GetValue() != ambient)
    {
    int old_disable = this->AmbientScale->GetDisableCommands();
    this->AmbientScale->SetDisableCommands(1);
    this->AmbientScale->SetValue(ambient);
    this->AmbientScale->SetDisableCommands(old_disable);
    }

  // Diffuse

  if (this->DiffuseScale && this->DiffuseScale->GetValue() != diffuse)
    {
    int old_disable = this->DiffuseScale->GetDisableCommands();
    this->DiffuseScale->SetDisableCommands(1);
    this->DiffuseScale->SetValue(diffuse);
    this->DiffuseScale->SetDisableCommands(old_disable);
    }

  // Specular

  if (this->SpecularScale && this->SpecularScale->GetValue() != specular)
    {
    int old_disable = this->SpecularScale->GetDisableCommands();
    this->SpecularScale->SetDisableCommands(1);
    this->SpecularScale->SetValue(specular);
    this->SpecularScale->SetDisableCommands(old_disable);
    }

  // Specular power

  if (this->SpecularPowerScale && 
      this->SpecularPowerScale->GetValue() != specular_power)
    {
    int old_disable = this->SpecularPowerScale->GetDisableCommands();
    this->SpecularPowerScale->SetDisableCommands(1);
    this->SpecularPowerScale->SetValue(specular_power);
    this->SpecularPowerScale->SetDisableCommands(old_disable);
    }
}

//----------------------------------------------------------------------------
int vtkKWMaterialPropertyWidget::UpdateScalesFromPreset(const Preset *preset)
{
  if (!preset)
    {
    return 0;
    }

  this->UpdateScales(preset->Ambient * 100.0, 
                     preset->Diffuse * 100.0,
                     preset->Specular * 100.0,
                     preset->SpecularPower * 100.0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::UpdatePreview()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Update the preview

  int pixel_size = 3 + (this->GridOpacity == 1.0 ? 0 : 1);

  unsigned char *buffer = 
    new unsigned char [this->PreviewSize * this->PreviewSize * pixel_size];

  this->CreateImage(buffer, 
                    this->AmbientScale->GetValue(),
                    this->DiffuseScale->GetValue(),
                    this->SpecularScale->GetValue(),
                    this->SpecularPowerScale->GetValue(), 
                    this->PreviewSize);

  this->PreviewLabel->GetWidget()->SetImageToPixels(
    buffer, this->PreviewSize, this->PreviewSize, pixel_size);
  
  delete [] buffer;

  // Update the popup preview too automatically

  this->UpdatePopupPreview();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPopupPreviewSize(int v)
{
  if (this->PopupPreviewSize == v || v < 8)
    {
    return;
    }

  this->PopupPreviewSize = v;
  this->Modified();

  this->UpdatePopupPreview();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::UpdatePopupPreview()
{
  if (!this->IsCreated() || !this->PopupMode)
    {
    return;
    }

  // Update the popup preview

  int pixel_size = 3 + (this->GridOpacity == 1.0 ? 0 : 1);

  unsigned char *buffer = new unsigned char [
    this->PopupPreviewSize * this->PopupPreviewSize * pixel_size];

  this->CreateImage(buffer, 
                    this->AmbientScale->GetValue(),
                    this->DiffuseScale->GetValue(),
                    this->SpecularScale->GetValue(),
                    this->SpecularPowerScale->GetValue(), 
                    this->PopupPreviewSize);

  this->PopupButton->GetWidget()->SetImageToPixels(
    buffer, this->PopupPreviewSize, this->PopupPreviewSize, pixel_size);
  
  delete [] buffer;
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPresetSize(int v)
{
  if (this->PresetSize == v || v < 8)
    {
    return;
    }

  this->PresetSize = v;
  this->Modified();

  this->CreatePresets();
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::CreatePresets()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Delete all presets

  vtkKWPushButtonSet *pbs = this->PresetPushButtonSet->GetWidget();
  pbs->DeleteAllWidgets();

  int pixel_size = 3 + (this->GridOpacity == 1.0 ? 0 : 1);

  unsigned char *buffer = 
    new unsigned char [this->PresetSize * this->PresetSize * pixel_size];

  // Create all presets

  int rank = 0;
  vtkKWMaterialPropertyWidgetInternals::PresetsContainerIterator it = 
    this->Internals->Presets.begin();
  vtkKWMaterialPropertyWidgetInternals::PresetsContainerIterator end = 
    this->Internals->Presets.end();
  for (; it != end; ++it, ++rank)
    {
    if (*it)
      {
      vtkKWPushButton *pb = pbs->AddWidget(rank);
      if ((*it)->HelpString)
        {
        pb->SetBalloonHelpString((*it)->HelpString);
        }
      
      this->CreateImage(buffer, 
                        (*it)->Ambient * 100.0, 
                        (*it)->Diffuse * 100.0,
                        (*it)->Specular * 100.0, 
                        (*it)->SpecularPower, 
                        this->PresetSize);

      pb->SetImageToPixels(
        buffer, this->PresetSize, this->PresetSize, pixel_size);
      
      ostrstream preset_callback;
      preset_callback << "PresetMaterialCallback " << rank << ends;
      pb->SetCommand(this, preset_callback.str());
      preset_callback.rdbuf()->freeze(0);
      }
    }

  delete [] buffer;

}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->PopupButton);
  this->PropagateEnableState(this->MaterialPropertiesFrame);
  this->PropagateEnableState(this->ControlFrame);
  this->PropagateEnableState(this->LightingFrame);

  int enabled = this->AreControlsEnabled() ? this->GetEnabled() : 0;

  if (this->AmbientScale)
    {
    this->AmbientScale->SetEnabled(enabled);
    }

  if (this->DiffuseScale)
    {
    this->DiffuseScale->SetEnabled(enabled);
    }

  if (this->SpecularScale)
    {
    this->SpecularScale->SetEnabled(enabled);
    }

  if (this->SpecularPowerScale)
    {
    this->SpecularPowerScale->SetEnabled(enabled);
    }

  if (this->PreviewLabel)
    {
    this->PreviewLabel->SetEnabled(enabled);
    }

  if (this->PresetPushButtonSet)
    {
    this->PresetPushButtonSet->SetEnabled(enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PropertyChangingCallback(double)
{
  // We put UpdatePreview out of the test to make sure the UI can still
  // be modified/tested without any real VTK object connected to it 
  // (say, a vtkVolumeProperty)

  int prop_has_changed = this->UpdatePropertyFromInterface();
  this->UpdatePreview();
  if (prop_has_changed)
    {
    this->InvokePropertyChangingCommand();
    this->SendStateEvent(this->PropertyChangingEvent);
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PropertyChangedCallback(double)
{
  // We put UpdatePreview out of the test to make sure the UI can still
  // be modified/tested without any real VTK object connected to it 
  // (say, a vtkVolumeProperty)

  this->UpdatePropertyFromInterface();

  this->UpdatePreview();

  // The ones above are out of the test too, since this callback is most of the
  // times called at the end of an interaction which had triggered 
  // PropertyChangingCallback already. Since PropertyChangingCallback will
  // modify the property, at the time PropertyChangedCallback is invoked
  // the property values are the same, thus UpdatePropertyFromInterface will
  // most likely return 0. We Sacrifice a bit of effiency here to make sure
  // the right command and events are send.

  this->InvokePropertyChangedCommand();
  this->SendStateEvent(this->PropertyChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PresetMaterialCallback(int rank)
{
  vtkKWMaterialPropertyWidgetInternals::PresetsContainerIterator it = 
    this->Internals->Presets.begin();
  vtkKWMaterialPropertyWidgetInternals::PresetsContainerIterator end = 
    this->Internals->Presets.end();
  for (; it != end && rank; ++it, --rank);

  if (it != end)
    {
    int prop_has_changed =  this->UpdatePropertyFromPreset(*it);
    this->UpdateScalesFromPreset(*it);
    this->Update();
    if (prop_has_changed)
      {
      this->InvokePropertyChangedCommand();
      this->SendStateEvent(this->PropertyChangedEvent);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SendStateEvent(int event)
{
  this->InvokeEvent(event, NULL);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::CreateImage(unsigned char *data,
                                              double ambient, 
                                              double diffuse,
                                              double specular,
                                              double specular_power,
                                              int size)
{  
  int i, j;
  double dist, intensity[3];
  double pt[3], normal[3], light[3], view[3], ref[3];
  double diffuseComp, specularComp, specularDot;
  int iGrid, jGrid;

  int pixel_size = 3 + (this->GridOpacity == 1.0 ? 0 : 1);
  int r, g, b, a;
  double size2 = (double)size * 0.5;

  for (i = 0; i < size; i++)
    {
    for (j = 0; j < size; j++)
      {
      dist = sqrt((double)((i-size2)*(i-size2) + (j-size2)*(j-size2)));
      if (dist <= size2 - 1)
        {
        normal[0] = pt[0] = (i-size2) / (size2-1);
        normal[1] = pt[1] = (j-size2) / (size2-1);
        normal[2] = pt[2] = sqrt(1 - pt[0]*pt[0] - pt[1]*pt[1]);
        vtkMath::Normalize(normal);

        light[0] = -5 - pt[0];
        light[1] = -5 - pt[1];
        light[2] = 5 - pt[2];
        vtkMath::Normalize(light);

        view[0] = -pt[0];
        view[1] = -pt[1];
        view[2] = 5 - pt[2];
        vtkMath::Normalize(view);
        
        ref[0] = 2*normal[0]*vtkMath::Dot(normal, light) - light[0];
        ref[1] = 2*normal[1]*vtkMath::Dot(normal, light) - light[1];
        ref[2] = 2*normal[2]*vtkMath::Dot(normal, light) - light[2];
        vtkMath::Normalize(ref);

        diffuseComp = diffuse*.01*vtkMath::Dot(normal, light);
        if (diffuseComp < 0)
          {
          diffuseComp = 0;
          }
        
        specularDot = vtkMath::Dot(ref, view);
        if (specularDot < 0)
          {
          specularDot = 0;
          }
        
        specularComp = specular*.01*pow(specularDot, specular_power);
        
        intensity[0] = (ambient*.01 + diffuseComp)*this->MaterialColor[0] 
          + specularComp;        
        intensity[1] = (ambient*.01 + diffuseComp)*this->MaterialColor[1] 
          + specularComp;
        intensity[2] = (ambient*.01 + diffuseComp)*this->MaterialColor[2] 
          + specularComp;
        if (intensity[0] > 1)
          {
          intensity[0] = 1;
          }
        if (intensity[1] > 1)
          {
          intensity[1] = 1;
          }
        if (intensity[2] > 1)
          {
          intensity[2] = 1;
          }
        
        r = static_cast<unsigned char>(255 * intensity[0]);
        g = static_cast<unsigned char>(255 * intensity[1]);
        b = static_cast<unsigned char>(255 * intensity[2]);
        a = 255;
        }
      else
        {
        a = (int)(this->GridOpacity * 255);
        iGrid = i / (size/8);
        jGrid = j / (size/8);
        
        if (((iGrid / 2) * 2 == iGrid &&
             (jGrid / 2) * 2 == jGrid) ||
            ((iGrid / 2) * 2 != iGrid &&
             (jGrid / 2) * 2 != jGrid))
          {
          r = g = b = 0;
          }
        else
          {
          r = g = b = 255;
          }
        }

      data[(i*size+j) * pixel_size] = r;
      data[(i*size+j) * pixel_size + 1] = g;
      data[(i*size+j) * pixel_size + 2] = b;
      if (pixel_size > 3)
        {
        data[(i*size+j) * pixel_size + 3] = a;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPropertyChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PropertyChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::InvokePropertyChangedCommand()
{
  this->InvokeObjectMethodCommand(this->PropertyChangedCommand);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPropertyChangingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PropertyChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::InvokePropertyChangingCommand()
{
  this->InvokeObjectMethodCommand(this->PropertyChangingCommand);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "PopupMode: " << (this->PopupMode ? "On" : "Off") << endl;
  os << indent << "PreviewSize: " << this->PreviewSize << endl;
  os << indent << "PresetSize: " << this->PresetSize << endl;
  os << indent << "PopupPreviewSize: " << this->PopupPreviewSize << endl;
  os << indent << "GridOpacity: " << this->GridOpacity << endl;
  os << indent << "PropertyChangedEvent: " 
     << this->PropertyChangedEvent << endl;
  os << indent << "PropertyChangingEvent: " 
     << this->PropertyChangingEvent << endl;
  os << indent << "MaterialColor: "
     << this->MaterialColor[0] << " "
     << this->MaterialColor[1] << " "
     << this->MaterialColor[2] << endl;
  os << indent << "LightingParametersVisibility: "
     << (this->LightingParametersVisibility ? "On" : "Off") << endl;

  os << indent << "PopupButton: ";
  if (this->PopupButton)
    {
    os << endl;
    this->PopupButton->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
