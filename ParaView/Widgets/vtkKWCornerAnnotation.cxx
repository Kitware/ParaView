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
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWLabel.h"
#include "vtkKWGenericComposite.h"
#include "vtkKWScale.h"
#include "vtkKWSerializer.h"
#include "vtkKWText.h"
#include "vtkKWTextProperty.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCornerAnnotation );
vtkCxxRevisionMacro(vtkKWCornerAnnotation, "1.29.2.11");

vtkSetObjectImplementationMacro(vtkKWCornerAnnotation,View,vtkKWView);

int vtkKWCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCornerAnnotation::vtkKWCornerAnnotation()
{
  this->View = NULL;
  
  this->CommandFunction = vtkKWCornerAnnotationCommand;
  this->CornerVisibilityButton = vtkKWCheckButton::New();
  this->CornerTopFrame = vtkKWWidget::New();
  this->CornerBottomFrame = vtkKWWidget::New();

  for (int i = 0; i < 4; i++)
    {
    this->CornerFrame[i] = vtkKWWidget::New();
    this->CornerLabel[i] = vtkKWWidget::New();    
    this->CornerText[i] = vtkKWText::New();
    }

  this->CornerProp = vtkCornerAnnotation::New();
  this->CornerProp->SetMaximumLineHeight(0.07);

  this->CornerComposite = vtkKWGenericComposite::New();
  this->CornerComposite->SetProp(this->CornerProp);

  this->MaximumLineHeightScale = vtkKWScale::New();

  this->TextPropertyWidget = vtkKWTextProperty::New();
}

//----------------------------------------------------------------------------
vtkKWCornerAnnotation::~vtkKWCornerAnnotation()
{
  this->CornerVisibilityButton->Delete();
  this->CornerTopFrame->Delete();
  this->CornerBottomFrame->Delete();

  for (int i = 0; i < 4; i++)
    {
    this->CornerFrame[i]->Delete();
    this->CornerLabel[i]->Delete();
    this->CornerText[i]->Delete();
    }

  this->CornerComposite->Delete();
  this->CornerProp->Delete();

  if (this->MaximumLineHeightScale)
    {
    this->MaximumLineHeightScale->Delete();
    this->MaximumLineHeightScale = NULL;
    }

  this->TextPropertyWidget->Delete();
  this->TextPropertyWidget = NULL;
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::ShowProperties()
{
  // unpack any current children
  this->Script("eval pack forget [pack slaves %s]",
               this->View->GetPropertiesParent()->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Close()
{
  if (this->CornerVisibilityButton->GetState())
    {
    this->View->RemoveComposite(this->CornerComposite);
    this->CornerVisibilityButton->SetState(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::Create(vtkKWApplication *app)
{
  this->vtkKWLabeledFrame::Create(app);
  
  this->CornerVisibilityButton->SetParent(this->GetFrame());
  this->CornerVisibilityButton->Create(this->Application,
                             "-text {Display corner annotation}");
  this->CornerVisibilityButton->SetBalloonHelpString(
    "Toggle the visibility of the corner annotation text");
  this->CornerVisibilityButton->SetCommand(this, "OnDisplayCorner");

  this->Script("pack %s -side top -padx 2 -pady 2 -anchor nw",
               this->CornerVisibilityButton->GetWidgetName());

  this->CornerTopFrame->SetParent( this->GetFrame() );
  this->CornerTopFrame->Create( app, "frame", "" );

  this->CornerBottomFrame->SetParent( this->GetFrame() );
  this->CornerBottomFrame->Create( app, "frame", "" );

  this->Script(
    "pack %s %s -side top -padx 2 -pady 2 -expand 1 -fill x -anchor nw",
    this->CornerTopFrame->GetWidgetName(),
    this->CornerBottomFrame->GetWidgetName() );

  int i;
  for (i = 0; i < 4; i++)
    {
    this->CornerFrame[i]->SetParent( 
      (i<2)?(this->CornerBottomFrame):(this->CornerTopFrame) );
    this->CornerLabel[i]->SetParent(this->CornerFrame[i]);
    this->CornerText[i]->SetParent(this->CornerFrame[i]);
    }

  this->CornerFrame[0]->Create( app, "frame", "" );
  this->CornerFrame[1]->Create( app, "frame", "" );

  this->Script(
    "pack %s %s -side left -padx 0 -pady 0 -expand 1 -fill x -anchor nw",
    this->CornerFrame[0]->GetWidgetName(),
    this->CornerFrame[1]->GetWidgetName() );

  this->CornerFrame[2]->Create( app, "frame", "" );
  this->CornerFrame[3]->Create( app, "frame", "" );

  this->Script(
    "pack %s %s -side left -padx 0 -pady 0 -expand 1 -fill x -anchor nw",
    this->CornerFrame[2]->GetWidgetName(),
    this->CornerFrame[3]->GetWidgetName() );

  this->CornerLabel[0]->Create(app,"label","-text {Lower left}");
  this->CornerLabel[1]->Create(app,"label","-text {Lower right}");
  this->CornerLabel[2]->Create(app,"label","-text {Upper left}");
  this->CornerLabel[3]->Create(app,"label","-text {Upper right}");

  for (i = 0; i < 4; i++)
    {
    this->CornerText[i]->Create(app,"-height 4 -width 10 -wrap none");
    this->Script("bind %s <Return> {%s CornerChanged %i}",
                 this->CornerText[i]->GetWidgetName(), 
                 this->GetTclName(), i);
    this->Script("bind %s <FocusOut> {%s CornerChanged %i}",
                 this->CornerText[i]->GetWidgetName(), 
                 this->GetTclName(), i);
    this->Script("pack %s -side top -anchor w -padx 4 -expand yes",
                 this->CornerLabel[i]->GetWidgetName());
    this->Script("pack %s -side top -anchor w -padx 4 -pady 2 -expand y -fill x",
                 this->CornerText[i]->GetWidgetName());
    }

  this->CornerLabel[0]->SetBalloonHelpString("Set the lower left corner annotation. The text will automatically scale to fit within the allocated space");

  this->CornerLabel[1]->SetBalloonHelpJustificationToRight();
  this->CornerLabel[1]->SetBalloonHelpString("Set the lower right corner annotation. The text will automatically scale to fit within the allocated space");

  this->CornerLabel[2]->SetBalloonHelpString("Set the upper left corner annotation. The text will automatically scale to fit within the allocated space");

  this->CornerLabel[3]->SetBalloonHelpJustificationToRight();
  this->CornerLabel[3]->SetBalloonHelpString("Set the upper right corner annotation. The text will automatically scale to fit within the allocated space");

  // Maximum line height

  this->MaximumLineHeightScale->SetParent(this->GetFrame());
  this->MaximumLineHeightScale->SetRange(0.01, 0.2);
  this->MaximumLineHeightScale->SetResolution(0.01);
  this->MaximumLineHeightScale->PopupScaleOn();
  this->MaximumLineHeightScale->Create(this->Application, "");
  this->MaximumLineHeightScale->SetValue(
    this->CornerProp->GetMaximumLineHeight());
  this->MaximumLineHeightScale->DisplayEntry();
  this->MaximumLineHeightScale->DisplayEntryAndLabelOnTopOff();
  this->MaximumLineHeightScale->DisplayLabel("Max line height:");
  this->Script("%s configure -width 5", 
               this->MaximumLineHeightScale->GetEntry()->GetWidgetName());
  this->MaximumLineHeightScale->SetBalloonHelpString("Set the maximum height of a line of text as a percentage of the vertical area allocated to this scaled text actor.");
  this->MaximumLineHeightScale->SetCommand(
    this, "MaximumLineHeightCallback");
  this->MaximumLineHeightScale->SetEndCommand(
    this, "MaximumLineHeightEndCallback");
  this->MaximumLineHeightScale->SetEntryCommand(
    this, "MaximumLineHeightEndCallback");

  this->Script("pack %s -padx 2 -pady 2 -side top -anchor w -fill y", 
               this->MaximumLineHeightScale->GetWidgetName());

  this->TextPropertyWidget->SetParent(this->GetFrame());
  this->TextPropertyWidget->SetTextProperty(this->CornerProp->GetTextProperty());
  this->TextPropertyWidget->SetActor2D(this->CornerProp);
  this->TextPropertyWidget->Create(this->Application);
  this->TextPropertyWidget->ShowLabelOn();
  this->TextPropertyWidget->GetLabel()->SetLabel("Properties:");
  this->TextPropertyWidget->SetTraceReferenceObject(this);
  this->TextPropertyWidget->SetTraceReferenceCommand("GetTextPropertyWidget");
  ostrstream onchangecommand;
  onchangecommand << this->GetTclName() << " OnTextChangeCallback" << ends;
  this->TextPropertyWidget->SetOnChangeCommand(onchangecommand.str());
  onchangecommand.rdbuf()->freeze(0);
  ostrstream oncolorchangecommand;
  oncolorchangecommand << this->GetTclName() << " OnTextColorChangeCallback" << ends;
  this->TextPropertyWidget->SetOnColorChangeCommand(oncolorchangecommand.str());
  oncolorchangecommand.rdbuf()->freeze(0);

  this->Script("pack %s -padx 2 -pady 2 -side top -anchor nw -fill y", 
               this->TextPropertyWidget->GetWidgetName());
}

//----------------------------------------------------------------------------
float vtkKWCornerAnnotation::GetMaximumLineHeight()
{
  if (this->MaximumLineHeightScale->IsCreated())
    {
    return this->MaximumLineHeightScale->GetValue();
    }
  return this->CornerProp->GetMaximumLineHeight();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetMaximumLineHeightNoTrace(float v)
{
  if (this->MaximumLineHeightScale->IsCreated())
    {
    this->MaximumLineHeightScale->SetValue(v);
    }
  this->CornerProp->SetMaximumLineHeight(v);
  if (this->CornerVisibilityButton->GetState())
    {
    this->View->Render();
    }
  this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetMaximumLineHeight(float v)
{
  this->SetMaximumLineHeightNoTrace(v);
  this->AddTraceEntry("$kw(%s) SetMaximumLineHeight %f",
                      this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::MaximumLineHeightCallback()
{
  if (this->MaximumLineHeightScale->IsCreated())
    {
    this->SetMaximumLineHeightNoTrace(this->MaximumLineHeightScale->GetValue());
    }
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::MaximumLineHeightEndCallback()
{
  if (this->MaximumLineHeightScale->IsCreated())
    {
    this->SetMaximumLineHeight(this->MaximumLineHeightScale->GetValue());
    }
}

//----------------------------------------------------------------------------
float *vtkKWCornerAnnotation::GetTextColor() 
{
  return this->TextPropertyWidget->GetColor();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetTextColor(float r, float g, float b)
{
  // The following call with eventually trigger the OnTextColorChangeCallback
  // and OnTextChangeCallback.

  this->TextPropertyWidget->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::OnTextColorChangeCallback()
{
  float *color = this->GetTextColor();
  this->InvokeEvent(vtkKWEvent::AnnotationColorChangedEvent, color);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::OnTextChangeCallback()
{
  this->View->Render();
  this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::OnDisplayCorner() 
{
  if (this->CornerVisibilityButton->GetState())
    {
    this->View->AddComposite(this->CornerComposite);
    for (int i = 0; i < 4; i++)
      {
      this->CornerProp->SetText(i,this->GetCornerText(i));
      }
    this->View->Render();
    this->AddTraceEntry("$kw(%s) SetVisibility 1", this->GetTclName());
    }
  else
    {
    this->View->RemoveComposite(this->CornerComposite);
    this->View->Render();
    this->AddTraceEntry("$kw(%s) SetVisibility 0", this->GetTclName());
    }
  this->InvokeEvent( vtkKWEvent::ViewAnnotationChangedEvent, 0 );
}

//----------------------------------------------------------------------------
int vtkKWCornerAnnotation::GetVisibility() 
{
  return this->CornerVisibilityButton->GetState();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetVisibility(int state) 
{
  if (state == this->CornerVisibilityButton->GetState())
    {
    return;
    }
  this->CornerVisibilityButton->SetState(state);
  this->OnDisplayCorner();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SetCornerText(const char *text, int corner) 
{
  this->CornerText[corner]->SetValue(text);
  this->CornerChanged(corner);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::CornerChanged(int i) 
{
  if (this->CornerText[i]->GetValue() &&
      this->CornerProp->GetText(i) && 
      !strcmp(this->CornerText[i]->GetValue(), this->CornerProp->GetText(i)))
    {
    return;
    }
  this->CornerProp->SetText(i,this->CornerText[i]->GetValue());
  if (this->CornerVisibilityButton->GetState())
    {
    this->View->Render();
    }
  this->InvokeEvent( vtkKWEvent::ViewAnnotationChangedEvent, 0 );

  this->AddTraceEntry("$kw(%s) SetCornerText {%s} %d", 
                      this->GetTclName(), this->CornerText[i]->GetValue(), i);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->vtkKWLabeledFrame::SerializeSelf(os,indent);

  os << indent << "CornerText0 ";
  vtkKWSerializer::WriteSafeString(os, this->CornerText[0]->GetValue());
  os << endl;
  os << indent << "CornerText1 ";
  vtkKWSerializer::WriteSafeString(os, this->CornerText[1]->GetValue());
  os << endl;
  os << indent << "CornerText2 ";
  vtkKWSerializer::WriteSafeString(os, this->CornerText[2]->GetValue());
  os << endl;
  os << indent << "CornerText3 ";
  vtkKWSerializer::WriteSafeString(os, this->CornerText[3]->GetValue());
  os << endl;

  os << indent << "CornerVisibilityButton " << this->CornerVisibilityButton->GetState() << endl;
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
    this->CornerVisibilityButton->SetState(i);
    this->OnDisplayCorner();
    return;
    }
  if (!strcmp(token,"CornerText0"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->CornerText[0]->SetValue(tmp);
    this->OnDisplayCorner();
    return;
    }
  if (!strcmp(token,"CornerText1"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->CornerText[1]->SetValue(tmp);
    this->OnDisplayCorner();
    return;
    }
  if (!strcmp(token,"CornerText2"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->CornerText[2]->SetValue(tmp);
    this->OnDisplayCorner();
    return;
    }
  if (!strcmp(token,"CornerText3"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->CornerText[3]->SetValue(tmp);
    this->OnDisplayCorner();
    return;
    }

  vtkKWLabeledFrame::SerializeToken(is,token);
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::SerializeRevision(ostream& os, vtkIndent indent)
{
  os << indent << "vtkKWCornerAnnotation ";
  this->ExtractRevision(os,"$Revision: 1.29.2.11 $");
  vtkKWLabeledFrame::SerializeRevision(os,indent);
}

//----------------------------------------------------------------------------
char *vtkKWCornerAnnotation::GetCornerText(int i)
{
  return this->CornerText[i]->GetValue();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerProp: " << this->GetCornerProp() << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "TextPropertyWidget: " << this->TextPropertyWidget << endl;
}
