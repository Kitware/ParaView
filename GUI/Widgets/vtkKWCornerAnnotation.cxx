/*=========================================================================

  Module:    vtkKWCornerAnnotation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWCornerAnnotation.h"

#include "vtkCornerAnnotation.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWPopupButtonLabeled.h"
#include "vtkKWTextLabeled.h"
#include "vtkKWPopupButton.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWScale.h"
#include "vtkKWText.h"
#include "vtkKWTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCornerAnnotation );
vtkCxxRevisionMacro(vtkKWCornerAnnotation, "1.86");

int vtkKWCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
                                 int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCornerAnnotation::vtkKWCornerAnnotation()
{
  this->CommandFunction = vtkKWCornerAnnotationCommand;

  this->AnnotationChangedEvent = vtkKWEvent::ViewAnnotationChangedEvent;

  // CornerAnnotation will either point to:
  // - InternalCornerAnnotation in vtkKWView mode, or
  // - vtkKWRenderWidget::GetCornerAnnotation() in vtkKWRenderWidget mode.

  this->CornerAnnotation = NULL;
  this->RenderWidget     = NULL;

  // If in vtkKWView mode, we need to create and maintain the corner prop and
  // composite

  // GUI

  this->PopupTextProperty                  = 0;

  this->CornerFrame             = vtkKWFrame::New();
  this->PropertiesFrame         = vtkKWFrame::New();
  this->MaximumLineHeightScale  = vtkKWScale::New();
  this->TextPropertyWidget      = vtkKWTextProperty::New();
  this->TextPropertyPopupButton = NULL;

  for (int i = 0; i < 4; i++)
    {
    this->CornerText[i] = vtkKWTextLabeled::New();
    }
}

//----------------------------------------------------------------------------
vtkKWCornerAnnotation::~vtkKWCornerAnnotation()
{
  // GUI

  if (this->CornerFrame)
    {
    this->CornerFrame->Delete();
    this->CornerFrame = NULL;
    }

  for (int i = 0; i < 4; i++)
    {
    if (this->CornerText[i])
      {
      this->CornerText[i]->Delete();
      this->CornerText[i] = NULL;
      }
    }

  if (this->PropertiesFrame)
    {
    this->PropertiesFrame->Delete();
    this->PropertiesFrame = NULL;
    }

  if (this->MaximumLineHeightScale)
    {
    this->MaximumLineHeightScale->Delete();
    this->MaximumLineHeightScale = NULL;
    }

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->Delete();
    this->TextPropertyWidget = NULL;
    }

  if (this->TextPropertyPopupButton)
    {
    this->TextPropertyPopupButton->Delete();
    this->TextPropertyPopupButton = NULL;
    }

  // CornerAnnotation was either pointing to:
  // - InternalCornerAnnotation in vtkKWView mode, or
  // - vtkKWRenderWidget::GetCornerAnnotation() in vtkKWRenderWidget mode.

  this->SetRenderWidget(NULL);

  // If in vtkKWView mode, we createed and maintained the corner prop and
  // composite

}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetRenderWidget(vtkKWRenderWidget *_arg)
{ 
  if (this->RenderWidget == _arg) 
    {
    return;
    }

  if (this->RenderWidget != NULL) 
    { 
    this->RenderWidget->UnRegister(this); 
    }

  this->RenderWidget = _arg;

  // We are now in vtkKWRenderWidget mode, the corner prop points to the
  // vtkKWRenderWidget's corner prop

  if (this->RenderWidget != NULL) 
    { 
    this->RenderWidget->Register(this); 
    this->CornerAnnotation = this->RenderWidget->GetCornerAnnotation();
    }
  else
    {
    this->CornerAnnotation = NULL;
    }

  this->Modified();

  // Update the GUI. Test if it is alive because we might be in the middle
  // of destructing the whole GUI

  if (this->IsAlive())
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Close()
{
  this->SetVisibility(0);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Create(vtkKWApplication *app, 
                                   const char *args)
{
  // Create the superclass widgets

  if (this->IsCreated())
    {
    vtkErrorMacro("CornerAnnotation already created");
    return;
    }

  this->Superclass::Create(app, args);

  int popup_text_property = 
    this->PopupTextProperty && !this->PopupMode;

  // --------------------------------------------------------------
  // If in popup mode, modify the popup button

  if (this->PopupMode)
    {
    this->PopupButton->SetText("Edit...");
    }

  // --------------------------------------------------------------
  // Edit the labeled frame

  this->Frame->SetLabelText("Corner annotation");

  // --------------------------------------------------------------
  // Edit the check button (Annotation visibility)

  this->CheckButton->SetText("Display corner annotation");

  this->CheckButton->SetBalloonHelpString(
    "Toggle the visibility of the corner annotation text");

  // --------------------------------------------------------------
  // Corners text

  this->CornerFrame->SetParent(this->Frame->GetFrame());
  this->CornerFrame->Create(app, 0);

  this->Script("pack %s -side top -padx 2 -expand t -fill x -anchor nw",
               this->CornerFrame->GetWidgetName());

  int i;
  for (i = 0; i < 4; i++)
    {
    this->CornerText[i]->SetParent(this->CornerFrame);
    this->CornerText[i]->Create( app, 0);
    this->CornerText[i]->SetLabelPositionToTop();
    vtkKWText *text = this->CornerText[i]->GetWidget();
    text->SetHeight(3);
    text->SetWidth(25);
    text->SetWrapToNone();
    this->Script(
      "bind %s <Return> {%s CornerTextCallback %i}",
      text->GetTextWidget()->GetWidgetName(), this->GetTclName(), i);
    this->Script(
      "bind %s <FocusOut> {%s CornerTextCallback %i}",
      text->GetTextWidget()->GetWidgetName(), this->GetTclName(), i);
    }

  this->CornerText[0]->GetLabel()->SetText("Lower left:");
  this->CornerText[1]->GetLabel()->SetText("Lower right:");
  this->CornerText[2]->GetLabel()->SetText("Upper left:");
  this->CornerText[3]->GetLabel()->SetText("Upper right:");

  this->CornerText[0]->SetBalloonHelpString(
    "Set the lower left corner annotation. The text will automatically scale "
    "to fit within the allocated space");

  this->CornerText[1]->SetBalloonHelpJustificationToRight();
  this->CornerText[1]->SetBalloonHelpString(
    "Set the lower right corner annotation. The text will automatically scale "
    "to fit within the allocated space");

  this->CornerText[2]->SetBalloonHelpString(
    "Set the upper left corner annotation. The text will automatically scale "
    "to fit within the allocated space");

  this->CornerText[3]->SetBalloonHelpJustificationToRight();
  this->CornerText[3]->SetBalloonHelpString(
    "Set the upper right corner annotation. The text will automatically scale "
    "to fit within the allocated space");

  this->Script("grid %s %s -row 0 -sticky news -padx 2 -pady 0 -ipady 0",
               this->CornerText[2]->GetWidgetName(), 
               this->CornerText[3]->GetWidgetName());

  this->Script("grid %s %s -row 1 -sticky news -padx 2 -pady 0 -ipady 0",
               this->CornerText[0]->GetWidgetName(), 
               this->CornerText[1]->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1",
               this->CornerFrame->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 1",
               this->CornerFrame->GetWidgetName());

  // --------------------------------------------------------------
  // Properties frame

  this->PropertiesFrame->SetParent(this->Frame->GetFrame());
  this->PropertiesFrame->Create(app, 0);

  this->Script("pack %s -side top -padx 2 -expand t -fill both -anchor nw",
               this->PropertiesFrame->GetWidgetName());

  // --------------------------------------------------------------
  // Maximum line height

  this->MaximumLineHeightScale->SetParent(this->PropertiesFrame);
  this->MaximumLineHeightScale->SetRange(0.01, 0.2);
  this->MaximumLineHeightScale->SetResolution(0.01);
  this->MaximumLineHeightScale->PopupScaleOn();
  this->MaximumLineHeightScale->Create(app, "");
  this->MaximumLineHeightScale->DisplayEntry();
  this->MaximumLineHeightScale->DisplayEntryAndLabelOnTopOff();
  this->MaximumLineHeightScale->DisplayLabel("Max line height:");
  this->MaximumLineHeightScale->SetEntryWidth(5);

  this->MaximumLineHeightScale->SetBalloonHelpString(
    "Set the maximum height of a line of text as a percentage of the vertical "
    "area allocated to this scaled text actor.");

  this->MaximumLineHeightScale->SetCommand(
    this, "MaximumLineHeightCallback");

  this->MaximumLineHeightScale->SetEndCommand(
    this, "MaximumLineHeightEndCallback");

  this->MaximumLineHeightScale->SetEntryCommand(
    this, "MaximumLineHeightEndCallback");

  this->Script("pack %s -padx 2 -pady 2 -side %s -anchor w -fill y", 
               this->MaximumLineHeightScale->GetWidgetName(),
               (popup_text_property ? "left" : "top"));
  
  // --------------------------------------------------------------
  // Text property : popup button if needed

  if (popup_text_property)
    {
    if (!this->TextPropertyPopupButton)
      {
      this->TextPropertyPopupButton = vtkKWPopupButtonLabeled::New();
      }
    this->TextPropertyPopupButton->SetParent(this->PropertiesFrame);
    this->TextPropertyPopupButton->Create(app);
    this->TextPropertyPopupButton->GetLabel()->SetText("Text properties:");
    this->TextPropertyPopupButton->GetWidget()->SetText("Edit...");
    this->Script("%s configure -bd 2 -relief groove", 
                 this->TextPropertyPopupButton->GetWidget()
                 ->GetPopupFrame()->GetWidgetName());

    this->Script("pack %s -padx 2 -pady 2 -side left -anchor w", 
                 this->TextPropertyPopupButton->GetWidgetName());

    this->TextPropertyWidget->SetParent(
      this->TextPropertyPopupButton->GetWidget()->GetPopupFrame());
    }
  else
    {
    this->TextPropertyWidget->SetParent(this->PropertiesFrame);
    }

  // --------------------------------------------------------------
  // Text property

  this->TextPropertyWidget->LongFormatOn();
  this->TextPropertyWidget->LabelOnTopOn();
  this->TextPropertyWidget->ShowLabelOn();
  this->TextPropertyWidget->Create(app);
  this->TextPropertyWidget->GetLabel()->SetText("Text properties:");
  this->TextPropertyWidget->SetTraceReferenceObject(this);
  this->TextPropertyWidget->SetTraceReferenceCommand("GetTextPropertyWidget");
  this->TextPropertyWidget->SetChangedCommand(this, "TextPropertyCallback");

  this->Script("pack %s -padx 2 -pady %d -side top -anchor nw -fill y", 
               this->TextPropertyWidget->GetWidgetName(),
               this->TextPropertyWidget->GetLongFormat() ? 0 : 2);

  // --------------------------------------------------------------
  // Update the GUI according to the Ivar value (i.e. the corner prop, etc.)

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Update()
{
  this->Superclass::Update();

  // If no widget or view, let's disable everything

  if (!this->RenderWidget)
    {
    this->SetEnabled(0);
    }

  if (!this->IsCreated())
    {
    return;
    }

  // Corners text

  for (int i = 0; i < 4; i++)
    {
    if (this->CornerText[i])
      {
      this->CornerText[i]->GetWidget()->SetValue(
        this->CornerAnnotation ? this->CornerAnnotation->GetText(i) : "");
      }
    }

  // Maximum line height

  if (this->MaximumLineHeightScale && this->CornerAnnotation)
    {
    this->MaximumLineHeightScale->SetValue(
      this->CornerAnnotation->GetMaximumLineHeight());
    }

  // Text property

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetTextProperty(
      this->CornerAnnotation ? this->CornerAnnotation->GetTextProperty():NULL);
    this->TextPropertyWidget->SetActor2D(this->CornerAnnotation);
    this->TextPropertyWidget->Update();
    }

  if (this->CheckButton && this->CornerAnnotation)
    {
    this->CheckButton->SetState(this->CornerAnnotation->GetVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Render() 
{
  if (this->RenderWidget)
    {
    this->RenderWidget->Render();
    }
}

//----------------------------------------------------------------------------
int vtkKWCornerAnnotation::GetVisibility() 
{
  // Note that the visibility here is based on the real visibility of the
  // annotation, not the state of the checkbutton

  return (this->CornerAnnotation &&
          this->CornerAnnotation->GetVisibility() &&
          this->RenderWidget &&
          this->RenderWidget->HasProp(this->CornerAnnotation)) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetVisibility(int state)
{
  // In vtkKWView mode, add/remove the composite
  // In vtkKWRenderWidget mode, add/remove the prop

  int old_visibility = this->GetVisibility();

  if (this->CornerAnnotation)
    {
    if (state)
      {
      this->CornerAnnotation->VisibilityOn();
      if (this->RenderWidget)
        {
        this->RenderWidget->SetCornerAnnotationVisibility(state);
        }
      }
    else
      {
      this->CornerAnnotation->VisibilityOff();
      if (this->RenderWidget)
        {
        this->RenderWidget->SetCornerAnnotationVisibility(state);
        }
      }
    }

  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->SendChangedEvent();
    this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), state);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::CheckButtonCallback() 
{
  if (this->CheckButton && this->CheckButton->IsCreated())
    {
    this->SetVisibility(this->CheckButton->GetState() ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetMaximumLineHeightNoTrace(float v)
{
  if (!this->CornerAnnotation ||
      this->CornerAnnotation->GetMaximumLineHeight() == v)
    {
    return;
    }

  this->CornerAnnotation->SetMaximumLineHeight(v);

  this->Update();

  if (this->GetVisibility())
    {
    this->Render();
    }

  this->SendChangedEvent();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetMaximumLineHeight(float v)
{
  this->SetMaximumLineHeightNoTrace(v);
  this->AddTraceEntry(
    "$kw(%s) SetMaximumLineHeight %f", this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::MaximumLineHeightCallback()
{
  if (this->IsCreated() && this->MaximumLineHeightScale)
    {
    this->SetMaximumLineHeightNoTrace(
      this->MaximumLineHeightScale->GetValue());
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::MaximumLineHeightEndCallback()
{
  if (this->IsCreated() && this->MaximumLineHeightScale)
    {
    this->SetMaximumLineHeight(this->MaximumLineHeightScale->GetValue());
    }
}

//----------------------------------------------------------------------------
double *vtkKWCornerAnnotation::GetTextColor() 
{
  if (this->TextPropertyWidget)
    {
    return this->TextPropertyWidget->GetColor();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetTextColor(double r, double g, double b)
{
  // The following call with eventually trigger the TextPropertyCallback 
  // (see Create()).

  double *rgb = this->GetTextColor();

  if (rgb && 
      (rgb[0] != r || rgb[1] != g || rgb[2] != b) &&
      this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::TextPropertyCallback()
{
  if (this->GetVisibility())
    {
    this->Render();
    }

  this->SendChangedEvent();
}

//----------------------------------------------------------------------------
char *vtkKWCornerAnnotation::GetCornerText(int i)
{
  if (this->CornerAnnotation)
    {
    return this->CornerAnnotation->GetText(i);
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetCornerText(const char *text, int corner) 
{
  if (this->CornerAnnotation &&
      (!this->GetCornerText(corner) ||
       strcmp(this->GetCornerText(corner), text)))
    {
    this->CornerAnnotation->SetText(corner, text);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();

    this->AddTraceEntry("$kw(%s) SetCornerText {%s} %d", 
                        this->GetTclName(), text, corner);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::CornerTextCallback(int i) 
{
  if (this->IsCreated() && this->CornerText[i])
    {
    this->SetCornerText(this->CornerText[i]->GetWidget()->GetValue(), i);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CornerFrame)
    {
    this->CornerFrame->SetEnabled(this->Enabled);
    }

  int i;
  for (i = 0; i < 4; i++)
    {
    if (this->CornerText[i])
      {
      this->CornerText[i]->SetEnabled(this->Enabled);
      }
    }

  if (this->PropertiesFrame)
    {
    this->PropertiesFrame->SetEnabled(this->Enabled);
    }

  if (this->MaximumLineHeightScale)
    {
    this->MaximumLineHeightScale->SetEnabled(this->Enabled);
    }

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetEnabled(this->Enabled);
    }

  if (this->TextPropertyPopupButton)
    {
    this->TextPropertyPopupButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SendChangedEvent()
{
  if (!this->CornerAnnotation)
    {
    return;
    }

  this->InvokeEvent(this->AnnotationChangedEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AnnotationChangedEvent: " 
     << this->AnnotationChangedEvent << endl;
  os << indent << "CornerAnnotation: " << this->GetCornerAnnotation() << endl;
  os << indent << "RenderWidget: " << this->GetRenderWidget() << endl;
  os << indent << "TextPropertyWidget: " << this->TextPropertyWidget << endl;
  os << indent << "MaximumLineHeightScale: " << this->MaximumLineHeightScale << endl;
  os << indent << "PopupTextProperty: " 
     << (this->PopupTextProperty ? "On" : "Off") << endl;
}

