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
#include "vtkKWCompositeCollection.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWGenericComposite.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledPopupButton.h"
#include "vtkKWLabeledText.h"
#include "vtkKWPopupButton.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWScale.h"
#include "vtkKWSerializer.h"
#include "vtkKWText.h"
#include "vtkKWTextProperty.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkString.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCornerAnnotation );
vtkCxxRevisionMacro(vtkKWCornerAnnotation, "1.79");

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
  this->View             = NULL;

  // If in vtkKWView mode, we need to create and maintain the corner prop and
  // composite

  this->InternalCornerComposite  = NULL;
  this->InternalCornerAnnotation = NULL;

  // GUI

  this->PopupTextProperty                  = 0;

  this->CornerFrame             = vtkKWFrame::New();
  this->PropertiesFrame         = vtkKWFrame::New();
  this->MaximumLineHeightScale  = vtkKWScale::New();
  this->TextPropertyWidget      = vtkKWTextProperty::New();
  this->TextPropertyPopupButton = NULL;

  for (int i = 0; i < 4; i++)
    {
    this->CornerText[i] = vtkKWLabeledText::New();
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
  this->SetView(NULL);

  // If in vtkKWView mode, we createed and maintained the corner prop and
  // composite

  if (this->InternalCornerComposite)
    {
    this->InternalCornerComposite->Delete();
    this->InternalCornerComposite = NULL;
    }

  if (this->InternalCornerAnnotation)
    {
    this->InternalCornerAnnotation->Delete();
    this->InternalCornerAnnotation = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetView(vtkKWView *_arg)
{ 
  if (this->View == _arg) 
    {
    return;
    }

  if (this->View != NULL) 
    { 
    this->View->UnRegister(this); 
    }

  this->View = _arg;

  // We are now in vtkKWView mode, create the corner prop and the composite

  if (this->View != NULL) 
    { 
    this->View->Register(this); 
    if (!this->InternalCornerAnnotation)
      {
      this->InternalCornerAnnotation = vtkCornerAnnotation::New();
      this->InternalCornerAnnotation->SetMaximumLineHeight(0.07);
      }
    if (!this->InternalCornerComposite)
      {
      this->InternalCornerComposite = vtkKWGenericComposite::New();
      }
    this->CornerAnnotation = this->InternalCornerAnnotation;
    }
  else
    {
    this->CornerAnnotation = NULL;
    }

  if (this->InternalCornerComposite)
    {
    this->InternalCornerComposite->SetProp(this->CornerAnnotation);
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
    this->PopupButton->SetLabel("Edit...");
    }

  // --------------------------------------------------------------
  // Edit the labeled frame

  this->Frame->SetLabel("Corner annotation");

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
    vtkKWText *text = this->CornerText[i]->GetText();
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

  this->CornerText[0]->SetLabel("Lower left:");
  this->CornerText[1]->SetLabel("Lower right:");
  this->CornerText[2]->SetLabel("Upper left:");
  this->CornerText[3]->SetLabel("Upper right:");

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
      this->TextPropertyPopupButton = vtkKWLabeledPopupButton::New();
      }
    this->TextPropertyPopupButton->SetParent(this->PropertiesFrame);
    this->TextPropertyPopupButton->Create(app);
    this->TextPropertyPopupButton->SetLabel("Text properties:");
    this->TextPropertyPopupButton->SetPopupButtonLabel("Edit...");
    this->Script("%s configure -bd 2 -relief groove", 
                 this->TextPropertyPopupButton->GetPopupButton()
                 ->GetPopupFrame()->GetWidgetName());

    this->Script("pack %s -padx 2 -pady 2 -side left -anchor w", 
                 this->TextPropertyPopupButton->GetWidgetName());

    this->TextPropertyWidget->SetParent(
      this->TextPropertyPopupButton->GetPopupButton()->GetPopupFrame());
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
  this->TextPropertyWidget->GetLabel()->SetLabel("Text properties:");
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

  if (!this->View && !this->RenderWidget)
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
      this->CornerText[i]->GetText()->SetValue(
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
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Render() 
{
  if (this->View)
    {
    this->View->Render();
    }

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
          ((this->View &&  
            this->View->HasComposite(this->InternalCornerComposite)) ||
           (this->RenderWidget &&
            this->RenderWidget->HasProp(this->CornerAnnotation)))) ? 1 : 0;
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
      if (this->View && 
          this->InternalCornerComposite &&
          !this->View->HasComposite(this->InternalCornerComposite))
        {
        this->View->Add2DComposite(this->InternalCornerComposite);
        }
      if (this->RenderWidget)
        {
        this->RenderWidget->SetCornerAnnotationVisibility(state);
        }
      }
    else
      {
      this->CornerAnnotation->VisibilityOff();
      if (this->View && 
          this->InternalCornerComposite &&
          this->View->HasComposite(this->InternalCornerComposite))
        {
        this->View->Remove2DComposite(this->InternalCornerComposite);
        }
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
    this->SetCornerText(this->CornerText[i]->GetText()->GetValue(), i);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeSelf(os,indent);

  os << indent << "CornerText0 ";
  vtkKWSerializer::WriteSafeString(os, this->GetCornerText(0));
  os << endl;
  os << indent << "CornerText1 ";
  vtkKWSerializer::WriteSafeString(os, this->GetCornerText(1));
  os << endl;
  os << indent << "CornerText2 ";
  vtkKWSerializer::WriteSafeString(os, this->GetCornerText(2));
  os << endl;
  os << indent << "CornerText3 ";
  vtkKWSerializer::WriteSafeString(os, this->GetCornerText(3));
  os << endl;

  os << indent << "CornerVisibilityButton " << this->GetVisibility() << endl;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeToken(istream& is, const char *token)
{
  this->Superclass::SerializeToken(is, token);

  int i;
  char tmp[VTK_KWSERIALIZER_MAX_TOKEN_LENGTH];
  
  if (!strcmp(token,"CornerVisibilityButton"))
    {
    is >> i;
    this->SetVisibility(i);
    return;
    }
  if (!strcmp(token,"CornerText0"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->SetCornerText(tmp, 0);
    return;
    }
  if (!strcmp(token,"CornerText1"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->SetCornerText(tmp, 1);
    return;
    }
  if (!strcmp(token,"CornerText2"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->SetCornerText(tmp, 2);
    return;
    }
  if (!strcmp(token,"CornerText3"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->SetCornerText(tmp, 3);
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeRevision(ostream& os, vtkIndent indent)
{
  os << indent << "vtkKWCornerAnnotation ";
  this->ExtractRevision(os,"$Revision: 1.79 $");
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
void vtkKWCornerAnnotation::SaveState(ofstream *file)
{
  *file << "$kw(" << this->GetTclName() << ") SetVisibility "
        << this->GetVisibility() << endl;
  
  int i;
  for (i = 0; i < 4; i++)
    {
    *file << "$kw(" << this->GetTclName() << ") SetCornerText {";
    if (this->GetCornerText(i))
      {
      *file << this->GetCornerText(i);
      }
    *file << "} " << i << endl;
    }
  
  *file << "$kw(" << this->GetTclName() << ") SetMaximumLineHeight "
        << this->GetCornerAnnotation()->GetMaximumLineHeight() << endl;
  
  *file << "set kw(" << this->TextPropertyWidget->GetTclName()
        << ") [$kw(" << this->GetTclName() << ") GetTextPropertyWidget]"
        << endl;
  char *tclName =
    new char[10 + strlen(this->TextPropertyWidget->GetTclName())];
  sprintf(tclName, "$kw(%s)", this->TextPropertyWidget->GetTclName());
  this->TextPropertyWidget->SaveInTclScript(file, tclName, 0);
  delete [] tclName;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AnnotationChangedEvent: " 
     << this->AnnotationChangedEvent << endl;
  os << indent << "CornerAnnotation: " << this->GetCornerAnnotation() << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "RenderWidget: " << this->GetRenderWidget() << endl;
  os << indent << "TextPropertyWidget: " << this->TextPropertyWidget << endl;
  os << indent << "MaximumLineHeightScale: " << this->MaximumLineHeightScale << endl;
  os << indent << "PopupTextProperty: " 
     << (this->PopupTextProperty ? "On" : "Off") << endl;
}

