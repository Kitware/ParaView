/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCornerAnnotation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWGenericComposite.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledText.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWScale.h"
#include "vtkKWSerializer.h"
#include "vtkKWText.h"
#include "vtkKWTextProperty.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCornerAnnotation );
vtkCxxRevisionMacro(vtkKWCornerAnnotation, "1.40");

int vtkKWCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCornerAnnotation::vtkKWCornerAnnotation()
{
  this->CommandFunction = vtkKWCornerAnnotationCommand;

  // CornerProp will either point to InternalCornerProp in vtkKWView mode, or
  // vtkKWRenderWidget::GetCornerProp() in vtkKWRenderWidget mode.

  this->CornerProp   = NULL;
  this->RenderWidget = NULL;
  this->View         = NULL;

  // If in vtkKWView mode, we need to create and maintain the corner prop and
  // composite

  this->InternalCornerComposite = NULL;
  this->InternalCornerProp      = NULL;

  // GUI

  this->CornerVisibilityButton = vtkKWCheckButton::New();

  this->CornerFrame = vtkKWFrame::New();
  for (int i = 0; i < 4; i++)
    {
    this->CornerText[i] = vtkKWLabeledText::New();
    }

  this->MaximumLineHeightScale = vtkKWScale::New();

  this->TextPropertyWidget = vtkKWTextProperty::New();
}

//----------------------------------------------------------------------------
vtkKWCornerAnnotation::~vtkKWCornerAnnotation()
{
  // CornerProp was either pointing to InternalCornerProp in vtkKWView mode, or
  // vtkKWRenderWidget::GetCornerProp() in vtkKWRenderWidget mode.

  this->SetRenderWidget(NULL);
  this->SetView(NULL);

  // If in vtkKWView mode, we createed and maintained the corner prop and
  // composite

  if (this->InternalCornerComposite)
    {
    this->InternalCornerComposite->Delete();
    this->InternalCornerComposite = NULL;
    }

  if (this->InternalCornerProp)
    {
    this->InternalCornerProp->Delete();
    this->InternalCornerProp = NULL;
    }

  // GUI

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
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetView(vtkKWView *_arg)
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this 
                << "): setting View to " << _arg ); 

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
    if (!this->InternalCornerProp)
      {
      this->InternalCornerProp = vtkCornerAnnotation::New();
      this->InternalCornerProp->SetMaximumLineHeight(0.07);
      }
    if (!this->InternalCornerComposite)
      {
      this->InternalCornerComposite = vtkKWGenericComposite::New();
      }
    this->CornerProp = this->InternalCornerProp;
    }
  else
    {
    this->CornerProp = NULL;
    }

  if (this->InternalCornerComposite)
    {
    this->InternalCornerComposite->SetProp(this->CornerProp);
    }

  this->Modified();

  // Update the GUI

  if (this->View)
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetRenderWidget(vtkKWRenderWidget *_arg)
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this 
                << "): setting RenderWidget to " << _arg ); 

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
    this->CornerProp = this->RenderWidget->GetCornerAnnotation();
    }
  else
    {
    this->CornerProp = NULL;
    }

  this->Modified();

  // Update the GUI

  if (this->RenderWidget)
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::ShowProperties()
{
  if (!this->IsCreated())
    {
    return;
    }

  // In vtkKWView mode, unpack any current children

  if (this->View && this->View->GetPropertiesParent())
    {
    this->Script("eval pack forget [pack slaves %s]",
                 this->View->GetPropertiesParent()->GetWidgetName());
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
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("CornerAnnotation already created");
    return;
    }

  // Call the superclass, this will set the application

  this->Superclass::Create(app, 0);

  // Annotation visibility

  this->CornerVisibilityButton->SetParent(this->GetFrame());
  this->CornerVisibilityButton->Create(this->Application, "");
  this->CornerVisibilityButton->SetText("Display corner annotation");
  this->CornerVisibilityButton->SetBalloonHelpString(
    "Toggle the visibility of the corner annotation text");
  this->CornerVisibilityButton->SetCommand(this, "DisplayCornerCallback");

  this->Script("pack %s -side top -padx 2 -pady 2 -anchor nw",
               this->CornerVisibilityButton->GetWidgetName());

  // Corners text

  this->CornerFrame->SetParent(this->GetFrame());
  this->CornerFrame->Create(app, 0);

  this->Script("pack %s -side top -padx 2 -pady 2 -expand t -fill x -anchor nw",
               this->CornerFrame->GetWidgetName());

  int i;
  for (i = 0; i < 4; i++)
    {
    this->CornerText[i]->SetParent(this->CornerFrame);
    this->CornerText[i]->Create( app, 0);
    this->Script("%s configure -height 4 -width 10 -wrap none",
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

  this->Script("grid %s %s -row 0 -sticky news -padx 2 -pady 2",
               this->CornerText[2]->GetWidgetName(), 
               this->CornerText[3]->GetWidgetName());

  this->Script("grid %s %s -row 1 -sticky news -padx 2 -pady 2",
               this->CornerText[0]->GetWidgetName(), 
               this->CornerText[1]->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1",
               this->CornerFrame->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 1",
               this->CornerFrame->GetWidgetName());

  // Maximum line height

  this->MaximumLineHeightScale->SetParent(this->GetFrame());
  this->MaximumLineHeightScale->SetRange(0.01, 0.2);
  this->MaximumLineHeightScale->SetResolution(0.01);
  this->MaximumLineHeightScale->PopupScaleOn();
  this->MaximumLineHeightScale->Create(this->Application, "");
  this->MaximumLineHeightScale->DisplayEntry();
  this->MaximumLineHeightScale->DisplayEntryAndLabelOnTopOff();
  this->MaximumLineHeightScale->DisplayLabel("Max line height:");
  this->MaximumLineHeightScale->GetEntry()->SetWidth(5);

  this->MaximumLineHeightScale->SetBalloonHelpString(
    "Set the maximum height of a line of text as a percentage of the vertical "
    "area allocated to this scaled text actor.");

  this->MaximumLineHeightScale->SetCommand(
    this, "MaximumLineHeightCallback");

  this->MaximumLineHeightScale->SetEndCommand(
    this, "MaximumLineHeightEndCallback");

  this->MaximumLineHeightScale->SetEntryCommand(
    this, "MaximumLineHeightEndCallback");

  this->Script("pack %s -padx 2 -pady 2 -side top -anchor w -fill y", 
               this->MaximumLineHeightScale->GetWidgetName());

  // Text property

  this->TextPropertyWidget->SetParent(this->GetFrame());
  this->TextPropertyWidget->LongFormatOn();
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

  ostrstream oncolorchangecommand;
  oncolorchangecommand << this->GetTclName() 
                       << " TextColorCallback" << ends;
  this->TextPropertyWidget->SetOnColorChangeCommand(oncolorchangecommand.str());
  oncolorchangecommand.rdbuf()->freeze(0);

  this->Script("pack %s -padx 2 -pady 2 -side top -anchor nw -fill y", 
               this->TextPropertyWidget->GetWidgetName());

  // Update the GUI according to the Ivar value (i.e. the corner prop, etc.)

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Update()
{
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
        this->CornerProp ? this->CornerProp->GetText(i) : "");
      }
    }

  // Maximum line height

  if (this->MaximumLineHeightScale && this->CornerProp)
    {
    this->MaximumLineHeightScale->SetValue(
      this->CornerProp->GetMaximumLineHeight());
    }

  // Text property

  if (this->TextPropertyWidget)
    {
    if (this->CornerProp)
      {
      this->TextPropertyWidget->SetTextProperty(
        this->CornerProp->GetTextProperty());
      }
    this->TextPropertyWidget->SetActor2D(this->CornerProp);
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
  // annotation, not the state of the checkbuttonMike Judge

  return (this->CornerProp &&
          this->CornerProp->GetVisibility() &&
          ((this->View &&  
            this->View->HasComposite(this->InternalCornerComposite)) ||
           (this->RenderWidget &&
            this->RenderWidget->HasProp(this->CornerProp)))) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetVisibility(int state)
{
  // In vtkKWView mode, add/remove the composite
  // In vtkKWRenderWidget mode, add/remove the prop

  int old_visibility = this->GetVisibility();

  if (this->CornerProp)
    {
    if (state)
      {
      if (this->View && 
          this->InternalCornerComposite &&
          !this->View->HasComposite(this->InternalCornerComposite))
        {
        this->View->AddComposite(this->InternalCornerComposite);
        }
      if (this->RenderWidget &&
          !this->RenderWidget->HasProp(this->CornerProp))
        {
        this->RenderWidget->AddProp(this->CornerProp);
        }
      }
    else
      {
      if (this->View && 
          this->InternalCornerComposite &&
          this->View->HasComposite(this->InternalCornerComposite))
        {
        this->View->RemoveComposite(this->InternalCornerComposite);
        }
      if (this->RenderWidget &&
          this->RenderWidget->HasProp(this->CornerProp))
        {
        this->RenderWidget->RemoveProp(this->CornerProp);
        }
      }
    }

  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
    this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), state);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::DisplayCornerCallback() 
{
  if (this->IsCreated() && this->CornerVisibilityButton)
    {
    this->SetVisibility(this->CornerVisibilityButton->GetState() ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetMaximumLineHeightNoTrace(float v)
{
  if (!this->CornerProp ||
      this->CornerProp->GetMaximumLineHeight() == v)
    {
    return;
    }

  this->CornerProp->SetMaximumLineHeight(v);

  this->Update();
  if (this->GetVisibility())
    {
    this->Render();
    }
  this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetMaximumLineHeight(float v)
{
  this->SetMaximumLineHeightNoTrace(v);
  this->AddTraceEntry("$kw(%s) SetMaximumLineHeight %f", this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::MaximumLineHeightCallback()
{
  if (this->IsCreated() && this->MaximumLineHeightScale)
    {
    this->SetMaximumLineHeightNoTrace(this->MaximumLineHeightScale->GetValue());
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
  // The following call with eventually trigger the TextColorCallback
  // and TextPropertyCallback (see Create()).

  float *rgb = this->GetTextColor();

  if (rgb && 
      (rgb[0] != r || rgb[1] != g || rgb[2] != b) &&
      this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::TextColorCallback()
{
  float *color = this->GetTextColor();
  this->InvokeEvent(vtkKWEvent::AnnotationColorChangedEvent, color);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::TextPropertyCallback()
{
  if (this->GetVisibility())
    {
    this->Render();
    }
  this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
}

//----------------------------------------------------------------------------
char *vtkKWCornerAnnotation::GetCornerText(int i)
{
  if (this->CornerProp)
    {
    return this->CornerProp->GetText(i);
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetCornerText(const char *text, int corner) 
{
  if (this->CornerProp &&
      (!this->GetCornerText(corner) ||
       strcmp(this->GetCornerText(corner), text)))
    {
    this->CornerProp->SetText(corner, text);

    this->Update();
    if (this->GetVisibility())
      {
      this->Render();
      }
    this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
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
  // invoke superclass
  this->vtkKWLabeledFrame::SerializeSelf(os,indent);

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
void vtkKWCornerAnnotation::SerializeToken(istream& is, 
                                           const char token[1024])
{
  int i;
  char tmp[1024];
  
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

  vtkKWLabeledFrame::SerializeToken(is,token);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeRevision(ostream& os, vtkIndent indent)
{
  os << indent << "vtkKWCornerAnnotation ";
  this->ExtractRevision(os,"$Revision: 1.40 $");
  vtkKWLabeledFrame::SerializeRevision(os,indent);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetEnabled(int e)
{
  // Propagate first (since objects can be modified externally, they might
  // not be in synch with this->Enabled)

  if (this->IsCreated())
    {
    if (this->CornerVisibilityButton)
      {
      this->CornerVisibilityButton->SetEnabled(e);
      }

    if (this->CornerFrame)
      {
      this->CornerFrame->SetEnabled(e);
      }

    int i;
    for (i = 0; i < 4; i++)
      {
      if (this->CornerText[i])
        {
        this->CornerText[i]->SetEnabled(e);
        }
      }

    if (this->MaximumLineHeightScale)
      {
      this->MaximumLineHeightScale->SetEnabled(e);
      }

    if (this->TextPropertyWidget)
      {
      this->TextPropertyWidget->SetEnabled(e);
      }
    }

  // Then call superclass, which will call SetEnabled on the label and 
  // update the internal Enabled ivar (although it is not of much use here)

  this->Superclass::SetEnabled(e);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerProp: " << this->GetCornerProp() << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "RenderWidget: " << this->GetRenderWidget() << endl;
  os << indent << "TextPropertyWidget: " << this->TextPropertyWidget << endl;
}
