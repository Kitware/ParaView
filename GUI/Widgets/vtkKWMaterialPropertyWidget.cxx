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

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLabelLabeled.h"
#include "vtkKWPopupButtonLabeled.h"
#include "vtkKWPushButtonSetLabeled.h"
#include "vtkKWPopupButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWScale.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkKWMaterialPropertyWidget, "1.4");

//----------------------------------------------------------------------------
vtkKWMaterialPropertyWidget::vtkKWMaterialPropertyWidget()
{
  this->PreviewSize      = 48;
  this->PresetSize       = 48;
  this->PopupPreviewSize = 24;
  this->GridOpacity      = 0.3;

  this->MaterialColor[0] = 1.0;
  this->MaterialColor[1] = 1.0;
  this->MaterialColor[2] = 1.0;
  
  this->PropertyChangedEvent  = vtkKWEvent::MaterialPropertyChangedEvent;
  this->PropertyChangingEvent = vtkKWEvent::MaterialPropertyChangingEvent;

  this->PropertyChangedCommand  = NULL;
  this->PropertyChangingCommand = NULL;

  // Presets

  this->Presets = vtkKWMaterialPropertyWidget::PresetsContainer::New();
  this->AddDefaultPresets();

  // UI

  this->PopupMode = 0;

  this->PopupButton = NULL;

  this->MaterialPropertiesFrame = vtkKWFrameLabeled::New();

  this->LightingFrame = vtkKWFrame::New();

  this->AmbientScale  = vtkKWScale::New();

  this->DiffuseScale  = vtkKWScale::New();

  this->SpecularScale = vtkKWScale::New();

  this->SpecularPowerScale = vtkKWScale::New();

  this->PresetsFrame = vtkKWFrame::New();

  this->PreviewLabel = vtkKWLabelLabeled::New();

  this->PresetPushButtonSet = vtkKWPushButtonSetLabeled::New();
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

  vtkKWMaterialPropertyWidget::Preset *preset = NULL;
  vtkKWMaterialPropertyWidget::PresetsContainerIterator *it = 
    this->Presets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(preset) == VTK_OK)
      {
      if (preset->HelpString)
        {
        delete [] preset->HelpString;
        }
      delete preset;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->Presets->Delete();
  
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
  vtkKWMaterialPropertyWidget::Preset *preset;

  // Presets : Unshaded

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 1.0;
  preset->Diffuse = 0.0;
  preset->Specular = 0.0;
  preset->SpecularPower = 1.0;
  preset->HelpString = kwsys::SystemTools::DuplicateString(
    "Full ambient eliminating all directional shading.");
  this->Presets->AppendItem(preset);

  // Presets : Dull

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 0.2;
  preset->Diffuse = 1.0;
  preset->Specular = 0.0;
  preset->SpecularPower = 1.0;
  preset->HelpString = kwsys::SystemTools::DuplicateString(
    "Dull material properties (no specular lighting)");
  this->Presets->AppendItem(preset);
  
  // Presets : Smooth

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 0.1;
  preset->Specular = 0.2;
  preset->Diffuse = 0.9;
  preset->SpecularPower = 10.0;
  preset->HelpString = kwsys::SystemTools::DuplicateString(
    "Smooth material properties (moderate specular lighting");
  this->Presets->AppendItem(preset);
  
  // Presets : Shiny

  preset = new vtkKWMaterialPropertyWidget::Preset;
  preset->Ambient = 0.1;
  preset->Diffuse = 0.6;
  preset->Specular = 0.5;
  preset->SpecularPower = 40.0;
  preset->HelpString = kwsys::SystemTools::DuplicateString(
    "Shiny material properties (high specular lighting)");
  this->Presets->AppendItem(preset);
} 

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::Create(vtkKWApplication *app,
                                         const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkKWFrame *frame;
  
  // --------------------------------------------------------------
  // If in popup mode, create the popup button

  if (this->PopupMode)
    {
    if (!this->PopupButton)
      {
      this->PopupButton = vtkKWPopupButtonLabeled::New();
      }
    
    this->PopupButton->SetParent(this);
    this->PopupButton->Create(app, 0);
    this->PopupButton->SetLabel("Edit material:");
    this->PopupButton->GetWidget()->SetLabel("");
    this->PopupButton->GetWidget()->SetPopupTitle("Material Properties");

    this->Script("pack %s -side left -anchor w -fill x",
                 this->PopupButton->GetWidgetName());
    }

  // --------------------------------------------------------------
  // Material frame

  if (this->PopupMode)
    {
    this->MaterialPropertiesFrame->ShowHideFrameOff();
    this->MaterialPropertiesFrame->SetParent(
      this->PopupButton->GetWidget()->GetPopupFrame());
    }
  else
    {
    this->MaterialPropertiesFrame->ShowHideFrameOn();
    this->MaterialPropertiesFrame->SetParent(this);
    }
  this->MaterialPropertiesFrame->Create(app, 0);
  this->MaterialPropertiesFrame->SetLabel("Material Properties");

  this->Script("pack %s -padx 0 -pady 0 -fill x -expand yes -anchor w",
               this->MaterialPropertiesFrame->GetWidgetName());
  
  frame = this->MaterialPropertiesFrame->GetFrame();

  // --------------------------------------------------------------
  // Lighting frame

  this->LightingFrame->SetParent(frame);
  this->LightingFrame->Create(app, 0);

  this->Script("pack %s -padx 0 -pady 0 -fill x -expand yes -anchor w",
               this->LightingFrame->GetWidgetName());
  
  // --------------------------------------------------------------
  // Ambient

  int entry_width = 4;
  int label_width = 12;
  int row = 0;

  this->AmbientScale->SetParent(this->LightingFrame);
  this->AmbientScale->Create(app, "");
  this->AmbientScale->DisplayEntryAndLabelOnTopOff();
  this->AmbientScale->SetCommand(this, "PropertyChangingCallback");
  this->AmbientScale->SetEndCommand(this, "PropertyChangedCallback");
  this->AmbientScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->AmbientScale->DisplayEntry();
  this->AmbientScale->SetEntryWidth(entry_width);
  this->AmbientScale->DisplayLabel("Ambient:");
  this->AmbientScale->SetLabelWidth(label_width);
  this->AmbientScale->SetRange(0, 100);
  this->AmbientScale->SetBalloonHelpString(
    "Set the ambient coefficient within the range [0,100] for lighting");
  
  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->AmbientScale->GetWidgetName(), row++);
  
  this->Script("grid columnconfigure %s 0 -weight 1",
               this->AmbientScale->GetParent()->GetWidgetName(), row++);
  
  // --------------------------------------------------------------
  // Diffuse

  this->DiffuseScale->SetParent(this->LightingFrame);
  this->DiffuseScale->Create(app, "");
  this->DiffuseScale->DisplayEntryAndLabelOnTopOff();
  this->DiffuseScale->SetCommand(this, "PropertyChangingCallback");
  this->DiffuseScale->SetEndCommand(this, "PropertyChangedCallback");
  this->DiffuseScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->DiffuseScale->DisplayEntry();
  this->DiffuseScale->SetEntryWidth(entry_width);
  this->DiffuseScale->DisplayLabel("Diffuse:");
  this->DiffuseScale->SetLabelWidth(label_width);
  this->DiffuseScale->SetRange(0, 100);
  this->DiffuseScale->SetBalloonHelpString(
    "Set the diffuse coefficient within the range [0,100] for lighting");
  
  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->DiffuseScale->GetWidgetName(), row++);
  
  // --------------------------------------------------------------
  // Specular

  this->SpecularScale->SetParent(this->LightingFrame);
  this->SpecularScale->Create(app, "");
  this->SpecularScale->DisplayEntryAndLabelOnTopOff();
  this->SpecularScale->SetCommand(this, "PropertyChangingCallback");
  this->SpecularScale->SetEndCommand(this, "PropertyChangedCallback");
  this->SpecularScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->SpecularScale->DisplayEntry();
  this->SpecularScale->SetEntryWidth(entry_width);
  this->SpecularScale->DisplayLabel("Specular:");
  this->SpecularScale->SetLabelWidth(label_width);
  this->SpecularScale->SetRange(0, 100);
  this->SpecularScale->SetBalloonHelpString(
    "Set the specular coefficient within the range [0,100] for lighting");

  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->SpecularScale->GetWidgetName(), row++);
 
  // --------------------------------------------------------------
  // Specular power

  this->SpecularPowerScale->SetParent(this->LightingFrame);
  this->SpecularPowerScale->Create(app, "");
  this->SpecularPowerScale->DisplayEntryAndLabelOnTopOff();
  this->SpecularPowerScale->SetCommand(this, "PropertyChangingCallback");
  this->SpecularPowerScale->SetEndCommand(this, "PropertyChangedCallback");
  this->SpecularPowerScale->SetEntryCommand(this, "PropertyChangedCallback");
  this->SpecularPowerScale->DisplayEntry();
  this->SpecularPowerScale->SetEntryWidth(entry_width);
  this->SpecularPowerScale->DisplayLabel("Power:");
  this->SpecularPowerScale->SetLabelWidth(label_width);
  this->SpecularPowerScale->SetRange(1, 50);
  this->SpecularPowerScale->SetBalloonHelpString(
    "Set the specular power within the range [0,50] for lighting");
  
  this->Script("grid %s -padx 2 -pady 2 -sticky news -row %d",
               this->SpecularPowerScale->GetWidgetName(), row++);
 
  // --------------------------------------------------------------
  // Presets + Preview frame

  this->PresetsFrame->SetParent(frame);
  this->PresetsFrame->Create(app, 0);
  
  this->Script("pack %s -anchor w -fill x -expand y",
               this->PresetsFrame->GetWidgetName());
  
  // --------------------------------------------------------------
  // Preview

  this->PreviewLabel->SetParent(this->PresetsFrame);
  this->PreviewLabel->SetLabelPositionToTop();
  this->PreviewLabel->ExpandWidgetOff();
  this->PreviewLabel->Create(app, "");
  this->PreviewLabel->SetLabel("Preview:");
  
  this->Script("pack %s -side left -padx 2 -pady 2 -anchor nw",
               this->PreviewLabel->GetWidgetName());
  
  // --------------------------------------------------------------
  // Presets

  this->PresetPushButtonSet->SetParent(this->PresetsFrame);
  this->PresetPushButtonSet->SetLabelPositionToTop();
  this->PresetPushButtonSet->SetLabel("Presets:");
  this->PresetPushButtonSet->Create(app);
  this->PresetPushButtonSet->ExpandWidgetOff();

  this->Script(
    "pack %s -side right -padx 2 -pady 2 -anchor nw",
    this->PresetPushButtonSet->GetWidgetName());

  vtkKWPushButtonSet *pbs = this->PresetPushButtonSet->GetWidget();
  pbs->PackHorizontallyOn();

  this->CreatePresets();

  // Update according to the current view/widget

  this->Update();
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
void vtkKWMaterialPropertyWidget::SetGridOpacity(float v)
{
  if (this->GridOpacity == v || v < 0.0 || v > 1.0)
    {
    return;
    }

  this->GridOpacity = v;
  this->Modified();

  this->UpdatePreview();
  this->UpdatePopupPreview();
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
  this->UpdatePopupPreview();
  this->CreatePresets();
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

  this->PreviewLabel->GetWidget()->SetImageOption(
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

  this->PopupButton->GetWidget()->SetImageOption(
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

  vtkIdType key = 0;
  vtkKWMaterialPropertyWidget::Preset *preset = NULL;
  vtkKWMaterialPropertyWidget::PresetsContainerIterator *it = 
    this->Presets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(preset) == VTK_OK && it->GetKey(key) == VTK_OK)
      {
      vtkKWPushButton *pb = pbs->AddWidget(key);
      if (preset->HelpString)
        {
        pb->SetBalloonHelpString(preset->HelpString);
        }
      
      this->CreateImage(buffer, 
                        preset->Ambient * 100.0, 
                        preset->Diffuse * 100.0,
                        preset->Specular * 100.0, 
                        preset->SpecularPower, 
                        this->PresetSize);

      pb->SetImageOption(
        buffer, this->PresetSize, this->PresetSize, pixel_size);
      
      ostrstream preset_callback;
      preset_callback << "PresetMaterialCallback " << key << ends;
      pb->SetCommand(this, preset_callback.str());
      preset_callback.rdbuf()->freeze(0);
      }
    it->GoToNextItem();
    }
  it->Delete();

  delete [] buffer;

}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PopupButton)
    {
    this->PopupButton->SetEnabled(this->Enabled);
    }

  if (this->MaterialPropertiesFrame)
    {
    this->MaterialPropertiesFrame->SetEnabled(this->Enabled);
    }

  if (this->LightingFrame)
    {
    this->LightingFrame->SetEnabled(this->Enabled);
    }

  if (this->AmbientScale)
    {
    this->AmbientScale->SetEnabled(this->Enabled);
    }

  if (this->DiffuseScale)
    {
    this->DiffuseScale->SetEnabled(this->Enabled);
    }

  if (this->SpecularScale)
    {
    this->SpecularScale->SetEnabled(this->Enabled);
    }

  if (this->SpecularPowerScale)
    {
    this->SpecularPowerScale->SetEnabled(this->Enabled);
    }

  if (this->PreviewLabel)
    {
    this->PreviewLabel->SetEnabled(this->Enabled);
    }

  if (this->PresetPushButtonSet)
    {
    this->PresetPushButtonSet->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PropertyChangingCallback()
{
  if (this->UpdatePropertyFromInterface())
    {
    this->UpdatePreview();
    this->InvokePropertyChangingCommand();
    this->SendStateEvent(this->PropertyChangingEvent);
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PropertyChangedCallback()
{
  if (this->UpdatePropertyFromInterface())
    {
    this->UpdatePreview();
    }

  // Those too are out of the test, because this callback is most of the times
  // called at the end of an interaction which had triggered 
  // PropertyChangingCallback until now. Since PropertyChangingCallback will
  // modify the property, at the time PropertyChangedCallback the property
  // values are the same, thus UpdatePropertyFromInterface will most likely
  // return 0. Sacrifice a bit of effiency here to make sure the right
  // command and event are send.

  this->InvokePropertyChangedCommand();
  this->SendStateEvent(this->PropertyChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::PresetMaterialCallback(vtkIdType key)
{
  vtkKWMaterialPropertyWidget::Preset *preset = NULL;
  if (this->Presets->GetItem(key, preset) != VTK_OK)
    {
    return;
    }

  if (this->UpdatePropertyFromPreset(preset))
    {
    this->Update();
    this->InvokePropertyChangedCommand();
    this->SendStateEvent(this->PropertyChangedEvent);
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SendStateEvent(int event)
{
  this->InvokeEvent(event, NULL);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::CreateImage(unsigned char *data,
                                              float ambient, 
                                              float diffuse,
                                              float specular,
                                              float specularPower,
                                              int size)
{  
  int i, j;
  float dist, intensity[3];
  float pt[3], normal[3], light[3], view[3], ref[3];
  float diffuseComp, specularComp, specularDot;
  int iGrid, jGrid;

  int pixel_size = 3 + (this->GridOpacity == 1.0 ? 0 : 1);
  int r, g, b, a;

  for (i = 0; i < size; i++)
    {
    for (j = 0; j < size; j++)
      {
      dist = sqrt((float)((i-size/2)*(i-size/2) + (j-size/2)*(j-size/2)));
      if (dist <= size/2 - 1)
        {
        normal[0] = pt[0] = (i-size/2) / (size/2.0-1);
        normal[1] = pt[1] = (j-size/2) / (size/2.0-1);
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
        
        specularComp = specular*.01*pow(specularDot, specularPower);
        
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
void vtkKWMaterialPropertyWidget::InvokeCommand(const char *command)
{
  if (command && *command)
    {
    this->Script("eval %s", command);
    }
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::InvokePropertyChangedCommand()
{
  this->InvokeCommand(this->PropertyChangedCommand);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::InvokePropertyChangingCommand()
{
  this->InvokeCommand(this->PropertyChangingCommand);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPropertyChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PropertyChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWMaterialPropertyWidget::SetPropertyChangingCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PropertyChangingCommand, object, method);
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
