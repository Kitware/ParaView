/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#ifndef DO_NOT_BUILD_XML_RW
#include "vtkXMLCornerAnnotationWriter.h"
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCornerAnnotation );
vtkCxxRevisionMacro(vtkKWCornerAnnotation, "1.66");

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

  this->PopupMode                          = 0;
  this->PopupTextProperty                  = 0;
  this->DisablePopupButtonWhenNotDisplayed = 0;

  this->PopupButton             = NULL;
  this->Frame                   = vtkKWLabeledFrame::New();
  this->CornerVisibilityButton  = vtkKWCheckButton::New();
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

  if (this->PopupButton)
    {
    this->PopupButton->Delete();
    this->PopupButton = NULL;
    }

  if (this->Frame)
    {
    this->Frame->Delete();
    this->Frame = NULL;
    }

  if (this->CornerVisibilityButton)
    {
    this->CornerVisibilityButton->Delete();
    this->CornerVisibilityButton = NULL;
    }
    
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
                                   const char* vtkNotUsed(args))
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("CornerAnnotation already created");
    return;
    }

  this->SetApplication(app);

  int popup_text_property = 
    this->PopupTextProperty && !this->PopupMode;

  // --------------------------------------------------------------
  // Create the container

  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  // --------------------------------------------------------------
  // If in popup mode, create the popup button

  if (this->PopupMode)
    {
    if (!this->PopupButton)
      {
      this->PopupButton = vtkKWPopupButton::New();
      }
    
    this->PopupButton->SetParent(this);
    this->PopupButton->Create(app, 0);
    this->PopupButton->SetLabel("Edit...");
    }

  // --------------------------------------------------------------
  // Create the labeled frame

  if (this->PopupMode)
    {
    this->Frame->ShowHideFrameOff();
    this->Frame->SetParent(this->PopupButton->GetPopupFrame());
    }
  else
    {
    this->Frame->SetParent(this);
    }
  this->Frame->Create(app, 0);
  this->Frame->SetLabel("Corner annotation");

  this->Script("pack %s -side top -anchor nw -fill both -expand y",
               this->Frame->GetWidgetName());

  // --------------------------------------------------------------
  // Annotation visibility

  if (this->PopupMode)
    {
    this->CornerVisibilityButton->SetParent(this);
    }
  else
    {
    this->CornerVisibilityButton->SetParent(this->Frame->GetFrame());
    }

  this->CornerVisibilityButton->Create(this->Application, "");

  this->CornerVisibilityButton->SetText("Display corner annotation");

  this->CornerVisibilityButton->SetBalloonHelpString(
    "Toggle the visibility of the corner annotation text");

  this->CornerVisibilityButton->SetCommand(this, "CornerVisibilityCallback");

  if (this->PopupMode)
    {
    this->Script("pack %s -side left -anchor w",
                 this->CornerVisibilityButton->GetWidgetName());
    this->Script("pack %s -side left -anchor w -fill x -expand t -padx 2",
                 this->PopupButton->GetWidgetName());
    }
  else
    {
    this->Script("pack %s -side top -padx 2 -anchor nw",
                 this->CornerVisibilityButton->GetWidgetName());
    }

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
    this->Script("%s configure -height 3 -width 25 -wrap none",
                 this->CornerText[i]->GetText()->GetWidgetName());
    this->Script("bind %s <Return> {%s CornerTextCallback %i}",
                 this->CornerText[i]->GetText()->GetWidgetName(), 
                 this->GetTclName(), i);
    this->Script("bind %s <FocusOut> {%s CornerTextCallback %i}",
                 this->CornerText[i]->GetText()->GetWidgetName(), 
                 this->GetTclName(), i);
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
  this->MaximumLineHeightScale->Create(this->Application, "");
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
    this->TextPropertyPopupButton->Create(this->Application);
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
  this->TextPropertyWidget->Create(this->Application);
  this->TextPropertyWidget->ShowLabelOn();
  this->TextPropertyWidget->GetLabel()->SetLabel("Text properties:");
  this->TextPropertyWidget->SetTraceReferenceObject(this);
  this->TextPropertyWidget->SetTraceReferenceCommand("GetTextPropertyWidget");

  ostrstream onchangecommand;
  onchangecommand << this->GetTclName() 
                  << " TextPropertyCallback" << ends;
  this->TextPropertyWidget->SetOnChangeCommand(onchangecommand.str());
  onchangecommand.rdbuf()->freeze(0);

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
  this->UpdateEnableState();

  // If no widget or view, let's disable everything

  if (!this->View && !this->RenderWidget)
    {
    this->SetEnabled(0);
    }

  if (!this->IsCreated())
    {
    return;
    }

  // Annotation visibility

  if (this->CornerVisibilityButton)
    {
    this->CornerVisibilityButton->SetState(this->GetVisibility());
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
    if (this->CornerAnnotation)
      {
      this->TextPropertyWidget->SetTextProperty(
        this->CornerAnnotation->GetTextProperty());
      }
    this->TextPropertyWidget->SetActor2D(this->CornerAnnotation);
    this->TextPropertyWidget->Update();
    }

  // Disable the popup button if not checked

  if (this->DisablePopupButtonWhenNotDisplayed &&
      this->PopupButton && 
      this->CornerVisibilityButton && 
      this->CornerVisibilityButton->IsCreated())
    {
    this->PopupButton->SetEnabled(
      this->CornerVisibilityButton->GetState() ? this->Enabled : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetDisablePopupButtonWhenNotDisplayed(
  int _arg)
{
  if (this->DisablePopupButtonWhenNotDisplayed == _arg)
    {
    return;
    }
  this->DisablePopupButtonWhenNotDisplayed = _arg;
  this->Modified();

  this->Update();
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
  // annotation, not the state of the checkbuttonMike Judge

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
        this->View->AddComposite(this->InternalCornerComposite);
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
        this->View->RemoveComposite(this->InternalCornerComposite);
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
void vtkKWCornerAnnotation::CornerVisibilityCallback() 
{
  if (this->IsCreated() && this->CornerVisibilityButton)
    {
    this->SetVisibility(this->CornerVisibilityButton->GetState() ? 1 : 0);
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
float *vtkKWCornerAnnotation::GetTextColor() 
{
  if (this->TextPropertyWidget)
    {
    return this->TextPropertyWidget->GetColor();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetTextColor(float r, float g, float b)
{
  // The following call with eventually trigger the TextPropertyCallback 
  // (see Create()).

  float *rgb = this->GetTextColor();

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

  this->Frame->SerializeToken(is,token);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeRevision(ostream& os, vtkIndent indent)
{
  os << indent << "vtkKWCornerAnnotation ";
  this->ExtractRevision(os,"$Revision: 1.66 $");
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PopupButton)
    {
    this->PopupButton->SetEnabled(this->Enabled);
    }

  if (this->Frame)
    {
    this->Frame->SetEnabled(this->Enabled);
    }

  if (this->CornerVisibilityButton)
    {
    this->CornerVisibilityButton->SetEnabled(this->Enabled);
    }

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

#ifdef DO_NOT_BUILD_XML_RW
  this->InvokeEvent(this->AnnotationChangedEvent, NULL);
#else
  ostrstream event;

  vtkXMLCornerAnnotationWriter *xmlw = vtkXMLCornerAnnotationWriter::New();
  xmlw->SetObject(this->CornerAnnotation);
  xmlw->WriteIndentedOff();
  xmlw->Write(event);
  xmlw->Delete();

  event << ends;

  this->InvokeEvent(this->AnnotationChangedEvent, event.str());
  event.rdbuf()->freeze(0);
#endif
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "CornerVisibilityButton: " 
     << this->CornerVisibilityButton << endl;
  os << indent << "AnnotationChangedEvent: " 
     << this->AnnotationChangedEvent << endl;
  os << indent << "CornerAnnotation: " << this->GetCornerAnnotation() << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "RenderWidget: " << this->GetRenderWidget() << endl;
  os << indent << "TextPropertyWidget: " << this->TextPropertyWidget << endl;
  os << indent << "MaximumLineHeightScale: " << this->MaximumLineHeightScale << endl;
  os << indent << "PopupMode: " 
     << (this->PopupMode ? "On" : "Off") << endl;
  os << indent << "PopupTextProperty: " 
     << (this->PopupTextProperty ? "On" : "Off") << endl;
  os << indent << "DisablePopupButtonWhenNotDisplayed: " 
     << (this->DisablePopupButtonWhenNotDisplayed ? "On" : "Off") << endl;
  os << indent << "PopupButton: " << this->PopupButton << endl;
}

