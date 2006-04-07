/*=========================================================================

  Module:    vtkKWHeaderAnnotationEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWHeaderAnnotationEditor.h"

#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWPopupButtonWithLabel.h"
#include "vtkKWPopupButton.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWTextPropertyEditor.h"
#include "vtkObjectFactory.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWHeaderAnnotationEditor );
vtkCxxRevisionMacro(vtkKWHeaderAnnotationEditor, "1.15");

//----------------------------------------------------------------------------
vtkKWHeaderAnnotationEditor::vtkKWHeaderAnnotationEditor()
{
  this->AnnotationChangedEvent  = vtkKWEvent::ViewAnnotationChangedEvent;
  this->PopupTextProperty       = 0;
  this->RenderWidget            = NULL;

  // GUI

  this->TextFrame               = vtkKWFrame::New();
  this->TextEntry               = vtkKWEntryWithLabel::New();
  this->TextPropertyWidget      = vtkKWTextPropertyEditor::New();
  this->TextPropertyPopupButton = NULL;
}

//----------------------------------------------------------------------------
vtkKWHeaderAnnotationEditor::~vtkKWHeaderAnnotationEditor()
{
  // GUI

  if (this->TextFrame)
    {
    this->TextFrame->Delete();
    this->TextFrame = NULL;
    }

  if (this->TextEntry)
    {
    this->TextEntry->Delete();
    this->TextEntry = NULL;
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

  this->SetRenderWidget(NULL);
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::SetRenderWidget(vtkKWRenderWidget *_arg)
{ 
  if (this->RenderWidget == _arg) 
    {
    return;
    }

  this->RenderWidget = _arg;
  this->Modified();

  // Update the GUI. Test if it is alive because we might be in the middle
  // of destructing the whole GUI

  if (this->IsAlive())
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::Create()
{
  // Create the superclass widgets

  if (this->IsCreated())
    {
    vtkErrorMacro("HeaderAnnotation already created");
    return;
    }

  this->Superclass::Create();

  int popup_text_property = 
    this->PopupTextProperty && !this->PopupMode;

  vtkKWWidget *frame = this->Frame->GetFrame();

  // --------------------------------------------------------------
  // If in popup mode, modify the popup button

  if (this->PopupMode)
    {
    this->PopupButton->SetText(ks_("Header Annotation Editor|Edit..."));
    }

  // --------------------------------------------------------------
  // Edit the labeled frame

  this->Frame->SetLabelText(ks_("Header Annotation Editor|Header annotation"));

  // --------------------------------------------------------------
  // Edit the check button (Annotation visibility)

  this->CheckButton->SetText(
    ks_("Header Annotation Editor|Display header annotation"));

  this->CheckButton->SetBalloonHelpString(
    k_("Toggle the visibility of the header annotation text"));

  // --------------------------------------------------------------
  // Text frame

  this->TextFrame->SetParent(frame);
  this->TextFrame->Create();

  this->Script("pack %s -side top -fill both -expand y", 
               this->TextFrame->GetWidgetName());
  
  // --------------------------------------------------------------
  // Header text

  this->TextEntry->SetParent(this->TextFrame);
  this->TextEntry->Create();
  this->TextEntry->GetLabel()->SetText(
    ks_("Header Annotation Editor|Header:"));
  this->TextEntry->GetWidget()->SetWidth(20);
  this->TextEntry->GetWidget()->SetCommand(this, "HeaderTextCallback");

  this->TextEntry->SetBalloonHelpString(
    k_("Set the header annotation. The text will automatically scale "
       "to fit within the allocated space"));

  this->Script("pack %s -padx 2 -pady 2 -side %s -anchor nw -expand y -fill x",
               this->TextEntry->GetWidgetName(),
               (popup_text_property ? "left" : "top"));
  
  // --------------------------------------------------------------
  // Text property : popup button if needed

  if (popup_text_property)
    {
    if (!this->TextPropertyPopupButton)
      {
      this->TextPropertyPopupButton = vtkKWPopupButtonWithLabel::New();
      }
    this->TextPropertyPopupButton->SetParent(this->TextFrame);
    this->TextPropertyPopupButton->Create();
    this->TextPropertyPopupButton->GetLabel()->SetText(
      ks_("Header Annotation Editor|Header properties:"));
    this->TextPropertyPopupButton->GetWidget()->SetText(
      ks_("Header Annotation Editor|Edit..."));

    vtkKWFrame *popupframe = 
      this->TextPropertyPopupButton->GetWidget()->GetPopupFrame();
    popupframe->SetBorderWidth(2);
    popupframe->SetReliefToGroove();

    this->Script("pack %s -padx 2 -pady 2 -side left -anchor w", 
                 this->TextPropertyPopupButton->GetWidgetName());

    this->TextPropertyWidget->SetParent(
      this->TextPropertyPopupButton->GetWidget()->GetPopupFrame());
    }
  else
    {
    this->TextPropertyWidget->SetParent(this->TextFrame);
    }

  // --------------------------------------------------------------
  // Text property

  this->TextPropertyWidget->LongFormatOn();
  this->TextPropertyWidget->LabelOnTopOn();
  this->TextPropertyWidget->LabelVisibilityOn();
  this->TextPropertyWidget->Create();
  this->TextPropertyWidget->GetLabel()->SetText(
    ks_("Header Annotation Editor|Header properties:"));
  this->TextPropertyWidget->SetChangedCommand(this, "TextPropertyCallback");

  this->Script("pack %s -padx 2 -pady %d -side top -anchor nw -fill y", 
               this->TextPropertyWidget->GetWidgetName(),
               this->TextPropertyWidget->GetLongFormat() ? 0 : 2);

  // --------------------------------------------------------------
  // Update the GUI according to the Ivar value (i.e. the corner prop, etc.)

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::Update()
{
  this->Superclass::Update();

  vtkTextActor *anno = NULL;
  if (this->RenderWidget)
    {
    anno = this->RenderWidget->GetHeaderAnnotation();
    }

  if (!this->IsCreated())
    {
    return;
    }

  // Corners text

  if (this->TextEntry)
    {
    if (anno)
      {
      this->TextEntry->GetWidget()->SetValue(
        anno->GetInput() ? anno->GetInput() : "");
      }
    }

  // Text property

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetTextProperty(
      anno ? anno->GetTextProperty() : NULL);
    this->TextPropertyWidget->SetActor2D(anno);
    this->TextPropertyWidget->Update();
    }
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::Render() 
{
  if (this->RenderWidget)
    {
    this->RenderWidget->Render();
    }
}

//----------------------------------------------------------------------------
int vtkKWHeaderAnnotationEditor::GetVisibility() 
{
  if (!this->RenderWidget)
    {
    return 0;
    }

  return this->RenderWidget->GetHeaderAnnotationVisibility();
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::SetVisibility(int state)
{
  if (!this->RenderWidget)
    {
    return;
    }

  int old_visibility = this->GetVisibility();
  this->RenderWidget->SetHeaderAnnotationVisibility(state);
  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->SendChangedEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::CheckButtonCallback(int state) 
{
  this->SetVisibility(state ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::TextPropertyCallback()
{
  if (this->GetVisibility())
    {
    this->Render();
    }

  this->SendChangedEvent();
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::SetHeaderText(const char *text) 
{
  if (this->RenderWidget && text &&
      (!this->RenderWidget->GetHeaderAnnotationText() || 
       strcmp(this->RenderWidget->GetHeaderAnnotationText(), text)))
    {
    this->RenderWidget->SetHeaderAnnotationText(text);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();
    }
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::HeaderTextCallback(const char *value) 
{
  this->SetHeaderText(value);
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  //int enabled = this->RenderWidget ? this->GetEnabled() : 0;
  int enabled = this->GetEnabled();

  if (this->TextFrame)
    {
    this->TextFrame->SetEnabled(enabled);
    }
  if (this->TextEntry)
    {
    this->TextEntry->SetEnabled(enabled);
    }
  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetEnabled(enabled);
    }
  if (this->TextPropertyPopupButton)
    {
    this->TextPropertyPopupButton->SetEnabled(enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::SendChangedEvent()
{
  if (!this->RenderWidget || !this->RenderWidget->GetHeaderAnnotation())
    {
    return;
    }

  this->InvokeEvent(this->AnnotationChangedEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWHeaderAnnotationEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AnnotationChangedEvent: " 
     << this->AnnotationChangedEvent << endl;
  os << indent << "RenderWidget: " << this->GetRenderWidget() << endl;
  os << indent << "PopupTextProperty: " 
     << (this->PopupTextProperty ? "On" : "Off") << endl;
}
