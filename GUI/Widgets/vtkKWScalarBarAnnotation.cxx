/*=========================================================================

  Module:    vtkKWScalarBarAnnotation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWScalarBarAnnotation.h"

#include "vtkColorTransferFunction.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWPopupButtonWithLabel.h"
#include "vtkKWPopupButton.h"
#include "vtkKWScalarComponentSelectionWidget.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWTextPropertyEditor.h"
#include "vtkKWThumbWheel.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkKWInternationalization.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkTextProperty.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWScalarBarAnnotation );
vtkCxxRevisionMacro(vtkKWScalarBarAnnotation, "1.32");

//----------------------------------------------------------------------------
vtkKWScalarBarAnnotation::vtkKWScalarBarAnnotation()
{
  this->AnnotationChangedEvent      = vtkKWEvent::ViewAnnotationChangedEvent;
  this->ScalarComponentChangedEvent = vtkKWEvent::ScalarComponentChangedEvent;

  this->PopupTextProperty       = 0;
  this->ScalarBarWidget         = NULL;
  this->VolumeProperty          = NULL;
  this->NumberOfComponents      = VTK_MAX_VRCOMP;
  this->LabelFormatVisibility         = 1;

  // GUI

  this->ComponentSelectionWidget = 
    vtkKWScalarComponentSelectionWidget::New();

  this->TitleFrame                      = vtkKWFrame::New();
  this->TitleEntry                      = vtkKWEntryWithLabel::New();
  this->TitleTextPropertyWidget         = vtkKWTextPropertyEditor::New();
  this->TitleTextPropertyPopupButton    = NULL;

  this->LabelFrame                      = vtkKWFrame::New();
  this->LabelFormatEntry                = vtkKWEntryWithLabel::New();
  this->LabelTextPropertyWidget         = vtkKWTextPropertyEditor::New();
  this->LabelTextPropertyPopupButton    = NULL;

  this->MaximumNumberOfColorsThumbWheel = vtkKWThumbWheel::New();
  this->NumberOfLabelsScale             = vtkKWScaleWithEntry::New();
}

//----------------------------------------------------------------------------
vtkKWScalarBarAnnotation::~vtkKWScalarBarAnnotation()
{
  // GUI

  if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->Delete();
    this->ComponentSelectionWidget = NULL;
    }

  if (this->TitleFrame)
    {
    this->TitleFrame->Delete();
    this->TitleFrame = NULL;
    }

  if (this->TitleEntry)
    {
    this->TitleEntry->Delete();
    this->TitleEntry = NULL;
    }

  if (this->TitleTextPropertyWidget)
    {
    this->TitleTextPropertyWidget->Delete();
    this->TitleTextPropertyWidget = NULL;
    }

  if (this->TitleTextPropertyPopupButton)
    {
    this->TitleTextPropertyPopupButton->Delete();
    this->TitleTextPropertyPopupButton = NULL;
    }

  if (this->LabelFrame)
    {
    this->LabelFrame->Delete();
    this->LabelFrame = NULL;
    }

  if (this->LabelFormatEntry)
    {
    this->LabelFormatEntry->Delete();
    this->LabelFormatEntry = NULL;
    }

  if (this->LabelTextPropertyWidget)
    {
    this->LabelTextPropertyWidget->Delete();
    this->LabelTextPropertyWidget = NULL;
    }

  if (this->LabelTextPropertyPopupButton)
    {
    this->LabelTextPropertyPopupButton->Delete();
    this->LabelTextPropertyPopupButton = NULL;
    }

  if (this->MaximumNumberOfColorsThumbWheel)
    {
    this->MaximumNumberOfColorsThumbWheel->Delete();
    this->MaximumNumberOfColorsThumbWheel = NULL;
    }

  if (this->NumberOfLabelsScale)
    {
    this->NumberOfLabelsScale->Delete();
    this->NumberOfLabelsScale = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetScalarBarWidget(vtkScalarBarWidget *_arg)
{ 
  if (this->ScalarBarWidget == _arg) 
    {
    return;
    }

  this->ScalarBarWidget = _arg;
  this->Modified();

  // Update the GUI. Test if it is alive because we might be in the middle
  // of destructing the whole GUI

  if (this->IsAlive())
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetVolumeProperty(
  vtkVolumeProperty *prop)
{
  if (this->VolumeProperty == prop)
    {
    return;
    }

  this->VolumeProperty = prop;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::CreateWidget()
{
  // Create the superclass widgets

  if (this->IsCreated())
    {
    vtkErrorMacro("ScalarBarAnnotation already created");
    return;
    }

  this->Superclass::CreateWidget();

  int popup_text_property = 
    this->PopupTextProperty && !this->PopupMode;

  vtkKWWidget *frame = this->Frame->GetFrame();

  // --------------------------------------------------------------
  // If in popup mode, modify the popup button

  if (this->PopupMode)
    {
    this->PopupButton->SetText(ks_("Scalar Bar Annotation Editor|Edit..."));
    }

  // --------------------------------------------------------------
  // Edit the labeled frame

  this->Frame->SetLabelText(ks_("Scalar Bar Annotation Editor|Scalar Bar"));

  // --------------------------------------------------------------
  // Edit the check button (Annotation visibility)

  this->CheckButton->SetText(
    ks_("Scalar Bar Annotation Editor|Display scalar bar"));

  this->CheckButton->SetBalloonHelpString(
    k_("Toggle the visibility of the scalar bar representing the mapping "
       "of scalar value to RGB color"));

  // --------------------------------------------------------------
  // Component selection

  this->ComponentSelectionWidget->SetParent(frame);
  this->ComponentSelectionWidget->Create();
  this->ComponentSelectionWidget->SetSelectedComponentChangedCommand(
    this, "SelectedComponentCallback");

  this->Script("pack %s -side top -padx 2 -pady 1 -anchor w", 
               this->ComponentSelectionWidget->GetWidgetName());

  // --------------------------------------------------------------
  // Title frame

  this->TitleFrame->SetParent(frame);
  this->TitleFrame->Create();

  this->Script("pack %s -side top -fill both -expand y", 
               this->TitleFrame->GetWidgetName());
  
  // --------------------------------------------------------------
  // Scalar Bar title

  this->TitleEntry->SetParent(this->TitleFrame);
  this->TitleEntry->Create();
  this->TitleEntry->GetLabel()->SetText("Title:");
  this->TitleEntry->GetWidget()->SetWidth(20);
  this->TitleEntry->GetWidget()->SetCommand(this, "ScalarBarTitleCallback");

  this->TitleEntry->SetBalloonHelpString(
    k_("Set the scalar bar title. The text will automatically scale "
       "to fit within the allocated space"));

  this->Script("pack %s -padx 2 -pady 2 -side %s -anchor nw -expand y -fill x",
               this->TitleEntry->GetWidgetName(),
               (!this->PopupMode ? "left" : "top"));
  
  // --------------------------------------------------------------
  // Scalar Bar title text property : popup button if needed

  if (popup_text_property)
    {
    if (!this->TitleTextPropertyPopupButton)
      {
      this->TitleTextPropertyPopupButton = vtkKWPopupButtonWithLabel::New();
      }
    this->TitleTextPropertyPopupButton->SetParent(this->TitleFrame);
    this->TitleTextPropertyPopupButton->Create();
    this->TitleTextPropertyPopupButton->GetLabel()->SetText(
      ks_("Scalar Bar Annotation Editor|Title properties:"));
    this->TitleTextPropertyPopupButton->GetWidget()->SetText(
      ks_("Scalar Bar Annotation Editor|Edit..."));

    vtkKWFrame *popupframe = 
      this->TitleTextPropertyPopupButton->GetWidget()->GetPopupFrame();
    popupframe->SetBorderWidth(2);
    popupframe->SetReliefToGroove();

    this->Script("pack %s -padx 2 -pady 2 -side left -anchor w", 
                 this->TitleTextPropertyPopupButton->GetWidgetName());

    this->TitleTextPropertyWidget->SetParent(
      this->TitleTextPropertyPopupButton->GetWidget()->GetPopupFrame());
    }
  else
    {
    this->TitleTextPropertyWidget->SetParent(this->TitleFrame);
    }

  // --------------------------------------------------------------
  // Scalar Bar title text property

  this->TitleTextPropertyWidget->LongFormatOn();
  this->TitleTextPropertyWidget->LabelOnTopOn();
  this->TitleTextPropertyWidget->LabelVisibilityOn();
  this->TitleTextPropertyWidget->Create();
  this->TitleTextPropertyWidget->GetLabel()->SetText(
    ks_("Scalar Bar Annotation Editor|Title properties:"));
  this->TitleTextPropertyWidget->SetChangedCommand(
    this, "TitleTextPropertyCallback");

  this->Script("pack %s -padx 2 -pady %d -side top -anchor nw -fill y", 
               this->TitleTextPropertyWidget->GetWidgetName(),
               this->TitleTextPropertyWidget->GetLongFormat() ? 0 : 2);

  // --------------------------------------------------------------
  // Label frame

  this->LabelFrame->SetParent(frame);
  this->LabelFrame->Create();

  this->Script("pack %s -side top -fill both -expand y -pady %d", 
               this->LabelFrame->GetWidgetName(),
               (this->PopupMode ? 6 : 0));
  
  // --------------------------------------------------------------
  // Scalar Bar label format

  this->LabelFormatEntry->SetParent(this->LabelFrame);
  this->LabelFormatEntry->Create();
  this->LabelFormatEntry->GetLabel()->SetText(
    ks_("Scalar Bar Annotation Editor|Label format:"));
  this->LabelFormatEntry->GetWidget()->SetWidth(20);
  this->LabelFormatEntry->GetWidget()->SetCommand(
    this, "ScalarBarLabelFormatCallback");

  this->LabelFormatEntry->SetBalloonHelpString(
    k_("Set the scalar bar label format."));

  // --------------------------------------------------------------
  // Scalar Bar label text property : popup button if needed

  if (popup_text_property)
    {
    if (!this->LabelTextPropertyPopupButton)
      {
      this->LabelTextPropertyPopupButton = vtkKWPopupButtonWithLabel::New();
      }
    this->LabelTextPropertyPopupButton->SetParent(this->LabelFrame);
    this->LabelTextPropertyPopupButton->Create();
    this->LabelTextPropertyPopupButton->GetLabel()->SetText(
      ks_("Scalar Bar Annotation Editor|Label properties:"));
    this->LabelTextPropertyPopupButton->GetWidget()->SetText(
      ks_("Scalar Bar Annotation Editor|Edit..."));

    vtkKWFrame *popupframe = 
      this->LabelTextPropertyPopupButton->GetWidget()->GetPopupFrame();
    popupframe->SetBorderWidth(2);
    popupframe->SetReliefToGroove();

    this->LabelTextPropertyWidget->SetParent(
      this->LabelTextPropertyPopupButton->GetWidget()->GetPopupFrame());
    }
  else
    {
    this->LabelTextPropertyWidget->SetParent(this->LabelFrame);
    }

  // --------------------------------------------------------------
  // Scalar Bar label text property

  this->LabelTextPropertyWidget->LongFormatOn();
  this->LabelTextPropertyWidget->LabelOnTopOn();
  this->LabelTextPropertyWidget->LabelVisibilityOn();
  this->LabelTextPropertyWidget->Create();
  this->LabelTextPropertyWidget->GetLabel()->SetText(
    ks_("Scalar Bar Annotation Editor|Label text properties:"));
  this->LabelTextPropertyWidget->SetChangedCommand(
    this, "LabelTextPropertyCallback");

  // --------------------------------------------------------------
  // Maximum number of colors

  vtkScalarBarActor *foo = vtkScalarBarActor::New();

  this->MaximumNumberOfColorsThumbWheel->SetParent(frame);
  this->MaximumNumberOfColorsThumbWheel->PopupModeOn();
  this->MaximumNumberOfColorsThumbWheel->SetMinimumValue(
    foo->GetMaximumNumberOfColorsMinValue());
  this->MaximumNumberOfColorsThumbWheel->ClampMinimumValueOn();
  this->MaximumNumberOfColorsThumbWheel->SetMaximumValue(2048);
    //    foo->GetMaximumNumberOfColorsMaxValue());
  this->MaximumNumberOfColorsThumbWheel->ClampMaximumValueOn();
  this->MaximumNumberOfColorsThumbWheel->SetResolution(1);
  this->MaximumNumberOfColorsThumbWheel->Create();
  this->MaximumNumberOfColorsThumbWheel->DisplayLabelOn();
  this->MaximumNumberOfColorsThumbWheel->GetLabel()->SetText(
    ks_("Scalar Bar Annotation Editor|Maximum number of colors:"));
  this->MaximumNumberOfColorsThumbWheel->DisplayEntryOn();
  this->MaximumNumberOfColorsThumbWheel->GetEntry()->SetWidth(5);

  this->MaximumNumberOfColorsThumbWheel->SetBalloonHelpString(
    k_("Set the maximum number of scalar bar segments to show."));

  this->MaximumNumberOfColorsThumbWheel->SetEndCommand(
    this, "MaximumNumberOfColorsEndCallback");

  this->MaximumNumberOfColorsThumbWheel->SetEntryCommand(
    this, "MaximumNumberOfColorsEndCallback");

  // --------------------------------------------------------------
  // Number of labels

  this->NumberOfLabelsScale->SetParent(frame);
  this->NumberOfLabelsScale->PopupModeOn();
  this->NumberOfLabelsScale->Create();
  this->NumberOfLabelsScale->SetRange(
    foo->GetNumberOfLabelsMinValue(), foo->GetNumberOfLabelsMaxValue());
  this->NumberOfLabelsScale->SetResolution(1);
  this->NumberOfLabelsScale->SetLabelText(
    ks_("Scalar Bar Annotation Editor|Number of labels:"));
  this->NumberOfLabelsScale->SetEntryWidth(5);

  this->NumberOfLabelsScale->SetBalloonHelpString(
    k_("Set the number of labels to show."));

  this->NumberOfLabelsScale->SetEndCommand(
    this, "NumberOfLabelsEndCallback");

  this->NumberOfLabelsScale->SetEntryCommand(
    this, "NumberOfLabelsEndCallback");

  foo->Delete();

  // --------------------------------------------------------------
  // Update the GUI according to the Ivar value (i.e. the corner prop, etc.)
  
  this->PackLabelFrameChildren();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::PackLabelFrameChildren()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->LabelFrame->UnpackChildren();
  
  if (this->LabelFormatEntry->IsCreated() && this->LabelFormatVisibility)
    {
    this->Script(
      "pack %s -padx 2 -pady 2 -side %s -anchor nw -expand y -fill x",
      this->LabelFormatEntry->GetWidgetName(),
      (!this->PopupMode ? "left" : "top"));
    }

  if (this->PopupTextProperty && 
      !this->PopupMode && 
      this->LabelTextPropertyPopupButton->IsCreated())
    {
    this->Script("pack %s -padx 2 -pady 2 -side left -anchor w", 
                 this->LabelTextPropertyPopupButton->GetWidgetName());
    }

  if (this->LabelTextPropertyWidget->IsCreated())
    {
    this->Script("pack %s -padx 2 -pady %d -side top -anchor nw -fill y", 
                 this->LabelTextPropertyWidget->GetWidgetName(),
                 this->LabelTextPropertyWidget->GetLongFormat() ? 0 : 2);
    }

  if (this->MaximumNumberOfColorsThumbWheel->IsCreated())
    {
    this->Script("pack %s -padx 2 -pady 2 -side top -anchor w -fill x", 
                 this->MaximumNumberOfColorsThumbWheel->GetWidgetName());
    }

  if (this->NumberOfLabelsScale->IsCreated())
    {
    this->Script("pack %s -padx 2 -pady 2 -side top -anchor w -fill x", 
                 this->NumberOfLabelsScale->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetLabelFormatVisibility(int arg)
{
  if (this->LabelFormatVisibility == arg)
    {
    return;
    }

  this->LabelFormatVisibility = arg;
  this->Modified();

  this->PackLabelFrameChildren();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::Update()
{
  this->Superclass::Update();

  vtkScalarBarActor *anno = NULL;

  if (!this->ScalarBarWidget)
    {
    this->SetEnabled(0);
    }
  else
    {
    anno = this->ScalarBarWidget->GetScalarBarActor();
    }

  if (!this->IsCreated())
    {
    return;
    }

  // Component selection menu

  if (this->ComponentSelectionWidget)
    {
    if (this->VolumeProperty)
      {
      this->ComponentSelectionWidget->SetIndependentComponents(
        this->VolumeProperty->GetIndependentComponents());
      this->ComponentSelectionWidget->SetNumberOfComponents(
        this->NumberOfComponents);
      this->ComponentSelectionWidget->AllowComponentSelectionOn();

      // Search inside the volume property to find which component we
      // are visualizing right now (and use it as selection)

      if (anno && anno->GetLookupTable())
        {
        for (int i = 0; i < VTK_MAX_VRCOMP; i++)
          {
          if (anno->GetLookupTable() == 
              this->VolumeProperty->GetRGBTransferFunction(i))
            {
            this->ComponentSelectionWidget->SetSelectedComponent(i);
            break;
            }
          }
        }
      }
    else
      {
      this->ComponentSelectionWidget->AllowComponentSelectionOff();
      }
    }
  
  // Title

  if (this->TitleEntry)
    {
    if (anno)
      {
      this->TitleEntry->GetWidget()->SetValue(
        anno->GetTitle() ? anno->GetTitle() : "");
      }
    }

  // Title text property

  if (this->TitleTextPropertyWidget)
    {
    this->TitleTextPropertyWidget->SetTextProperty(
      anno ? anno->GetTitleTextProperty() : NULL);
    this->TitleTextPropertyWidget->SetActor2D(anno);
    this->TitleTextPropertyWidget->Update();
    }

  // Label format

  if (this->LabelFormatEntry)
    {
    if (anno)
      {
      this->LabelFormatEntry->GetWidget()->SetValue(
        anno->GetLabelFormat() ? anno->GetLabelFormat() : "");
      }
    }

  // Label text property

  if (this->LabelTextPropertyWidget)
    {
    this->LabelTextPropertyWidget->SetTextProperty(
      anno ? anno->GetLabelTextProperty() : NULL);
    this->LabelTextPropertyWidget->SetActor2D(anno);
    this->LabelTextPropertyWidget->Update();
    }

  // Maximum number of colors

  if (this->MaximumNumberOfColorsThumbWheel)
    {
    if (anno)
      {
      this->MaximumNumberOfColorsThumbWheel->SetValue(
        anno->GetMaximumNumberOfColors());
      }
    }

  // Number of labels

  if (this->NumberOfLabelsScale)
    {
    if (anno)
      {
      this->NumberOfLabelsScale->SetValue(
        anno->GetNumberOfLabels());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::Render() 
{
  if (this->ScalarBarWidget && this->ScalarBarWidget->GetInteractor())
    {
    this->ScalarBarWidget->GetInteractor()->Render();
    }
}

//----------------------------------------------------------------------------
int vtkKWScalarBarAnnotation::GetVisibility() 
{
  if (!this->ScalarBarWidget)
    {
    return 0;
    }

  return this->ScalarBarWidget->GetEnabled();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetVisibility(int state)
{
  if (!this->ScalarBarWidget)
    {
    return;
    }

  int old_visibility = this->GetVisibility();
  this->ScalarBarWidget->SetEnabled(state);
  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->SendChangedEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetNumberOfComponents(int arg)
{
  if (this->NumberOfComponents == arg ||
      arg < 1 || arg > VTK_MAX_VRCOMP)
    {
    return;
    }

  this->NumberOfComponents = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::CheckButtonCallback(int state) 
{
  this->SetVisibility(state ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SelectedComponentCallback(int n)
{
  if (!this->VolumeProperty || 
      !this->ScalarBarWidget)
    {
    return;
    }

  vtkScalarBarActor *anno = this->ScalarBarWidget->GetScalarBarActor();
  if (!anno)
    {
    return;
    }

  anno->SetLookupTable(this->VolumeProperty->GetRGBTransferFunction(n));

  if (this->GetVisibility())
    {
    this->Render();
    }

  float val = n;
  this->InvokeEvent(ScalarComponentChangedEvent, &val);
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::TitleTextPropertyCallback()
{
  if (this->GetVisibility())
    {
    this->Render();
    }

  this->SendChangedEvent();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::LabelTextPropertyCallback()
{
  if (this->GetVisibility())
    {
    this->Render();
    }

  this->SendChangedEvent();
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetScalarBarTitle(const char *text) 
{
  if (this->ScalarBarWidget && text && 
      this->ScalarBarWidget->GetScalarBarActor() &&
      (!this->ScalarBarWidget->GetScalarBarActor()->GetTitle() || 
       strcmp(this->ScalarBarWidget->GetScalarBarActor()->GetTitle(), text)))
    {
    this->ScalarBarWidget->GetScalarBarActor()->SetTitle(text);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::ScalarBarTitleCallback(const char *value) 
{
  this->SetScalarBarTitle(value);
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SetScalarBarLabelFormat(const char *text) 
{
  if (this->ScalarBarWidget && text && 
      this->ScalarBarWidget->GetScalarBarActor() &&
      (!this->ScalarBarWidget->GetScalarBarActor()->GetLabelFormat() || 
       strcmp(this->ScalarBarWidget->GetScalarBarActor()->GetLabelFormat(), 
              text)))
    {
    this->ScalarBarWidget->GetScalarBarActor()->SetLabelFormat(text);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::ScalarBarLabelFormatCallback(const char *value) 
{
  this->SetScalarBarLabelFormat(value);
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::MaximumNumberOfColorsEndCallback(double value)
{
  if (this->ScalarBarWidget &&
      this->ScalarBarWidget->GetScalarBarActor())
    {
    int old_v = 
      this->ScalarBarWidget->GetScalarBarActor()->GetMaximumNumberOfColors();
    this->ScalarBarWidget->GetScalarBarActor()->SetMaximumNumberOfColors(
      static_cast<int>(value));
    if (old_v != 
        this->ScalarBarWidget->GetScalarBarActor()->GetMaximumNumberOfColors())
      {
      this->Update();
      this->Render();
      this->SendChangedEvent();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::NumberOfLabelsEndCallback(double value)
{
  if (this->ScalarBarWidget &&
      this->ScalarBarWidget->GetScalarBarActor())
    {
    int old_v = 
      this->ScalarBarWidget->GetScalarBarActor()->GetNumberOfLabels();
    this->ScalarBarWidget->GetScalarBarActor()->SetNumberOfLabels(
      static_cast<int>(value));
    if (old_v != 
        this->ScalarBarWidget->GetScalarBarActor()->GetNumberOfLabels())
      {
      this->Update();
      this->Render();
      this->SendChangedEvent();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ComponentSelectionWidget);
  this->PropagateEnableState(this->TitleFrame);
  this->PropagateEnableState(this->TitleEntry);
  this->PropagateEnableState(this->TitleTextPropertyWidget);
  this->PropagateEnableState(this->TitleTextPropertyPopupButton);
  this->PropagateEnableState(this->LabelFrame);
  this->PropagateEnableState(this->LabelFormatEntry);
  this->PropagateEnableState(this->LabelTextPropertyWidget);
  this->PropagateEnableState(this->LabelTextPropertyPopupButton);
  this->PropagateEnableState(this->MaximumNumberOfColorsThumbWheel);
  this->PropagateEnableState(this->NumberOfLabelsScale);
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::SendChangedEvent()
{
  if (!this->ScalarBarWidget || !this->ScalarBarWidget->GetScalarBarActor())
    {
    return;
    }

  this->InvokeEvent(this->AnnotationChangedEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWScalarBarAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AnnotationChangedEvent: " 
     << this->AnnotationChangedEvent << endl;
  os << indent << "ScalarComponentChangedEvent: " 
     << this->ScalarComponentChangedEvent << endl;
  os << indent << "ScalarBarWidget: " << this->GetScalarBarWidget() << endl;
  os << indent << "VolumeProperty: " << this->VolumeProperty << endl;
  os << indent << "PopupTextProperty: " 
     << (this->PopupTextProperty ? "On" : "Off") << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "LabelFormatVisibility: " << this->LabelFormatVisibility << endl;
}
