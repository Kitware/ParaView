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
#include "vtkKWWindow.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWText.h"
#include "vtkKWCheckButton.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWGenericComposite.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCornerAnnotation );

vtkSetObjectImplementationMacro(vtkKWCornerAnnotation,View,vtkKWView);

int vtkKWCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

vtkKWCornerAnnotation::vtkKWCornerAnnotation()
{
  this->View = NULL;
  
  this->CommandFunction = vtkKWCornerAnnotationCommand;

  this->CornerDisplayFrame = vtkKWWidget::New();
  this->CornerDisplayFrame->SetParent( this->GetFrame() );
  this->CornerButton = vtkKWCheckButton::New();
  this->CornerButton->SetParent(this->CornerDisplayFrame);
  this->CornerColor = vtkKWChangeColorButton::New();
  this->CornerColor->SetParent(this->CornerDisplayFrame);
  this->CornerTopFrame = vtkKWWidget::New();
  this->CornerTopFrame->SetParent( this->GetFrame() );
  this->CornerBottomFrame = vtkKWWidget::New();
  this->CornerBottomFrame->SetParent( this->GetFrame() );

  for (int i = 0; i < 4; i++)
    {
    this->CornerFrame[i] = vtkKWWidget::New();
    this->CornerFrame[i]->SetParent( 
	     (i<2)?(this->CornerBottomFrame):(this->CornerTopFrame) );
    this->CornerLabel[i] = vtkKWWidget::New();    
    this->CornerLabel[i]->SetParent(this->CornerFrame[i]);
    this->CornerText[i] = vtkKWText::New();
    this->CornerText[i]->SetParent(this->CornerFrame[i]);
    }

  this->CornerProp = vtkCornerAnnotation::New();
  this->CornerProp->SetMaximumLineHeight(0.07);
  this->CornerComposite = vtkKWGenericComposite::New();
  this->CornerComposite->SetProp(this->CornerProp);
}

vtkKWCornerAnnotation::~vtkKWCornerAnnotation()
{
  this->CornerDisplayFrame->Delete();
  this->CornerButton->Delete();
  this->CornerColor->Delete();
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

}

void vtkKWCornerAnnotation::ShowProperties()
{
  // unpack any current children
  this->Script("eval pack forget [pack slaves %s]",
               this->View->GetPropertiesParent()->GetWidgetName());
}

void vtkKWCornerAnnotation::Close()
{
  if (this->CornerButton->GetState())
    {
    this->View->RemoveComposite(this->CornerComposite);
    this->CornerButton->SetState(0);
    }
}

void vtkKWCornerAnnotation::Create(vtkKWApplication *app)
{
  this->vtkKWLabeledFrame::Create(app);
  
  this->CornerDisplayFrame->Create( app, "frame", "" );
  this->Script(
    "pack %s -side top -padx 0 -pady 0 -expand 1 -fill x -anchor nw",
    this->CornerDisplayFrame->GetWidgetName() );
  this->CornerButton->Create(this->Application,
                             "-text {Display Corner Annotation}");
  this->CornerButton->SetBalloonHelpString("Toggle the visibility of the corner annotation text");
  this->CornerButton->SetCommand(this, "OnDisplayCorner");
  this->Script("pack %s -side left -padx 2 -pady 4 -anchor nw",
               this->CornerButton->GetWidgetName());
  this->CornerColor->Create( app, "" );
  this->CornerColor->SetBalloonHelpJustificationToRight();
  this->CornerColor->SetBalloonHelpString( "Change the color of all four corner annotation text items" );
  this->Script("pack %s -side right -padx 2 -pady 4 -anchor ne",
               this->CornerColor->GetWidgetName());
  this->CornerColor->SetCommand( this, "SetTextColor" );

  this->CornerTopFrame->Create( app, "frame", "" );
  this->CornerBottomFrame->Create( app, "frame", "" );

  this->Script(
    "pack %s %s -side top -padx 2 -pady 2 -expand 1 -fill x -anchor nw",
    this->CornerTopFrame->GetWidgetName(),
    this->CornerBottomFrame->GetWidgetName() );

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

  this->CornerLabel[0]->Create(app,"label","-text {Lower Left}");
  this->CornerLabel[1]->Create(app,"label","-text {Lower Right}");
  this->CornerLabel[2]->Create(app,"label","-text {Upper Left}");
  this->CornerLabel[3]->Create(app,"label","-text {Upper Right}");

  for (int i = 0; i < 4; i++)
    {
    this->CornerText[i]->Create(app,"-height 4 -width 10 -wrap none");
    this->Script("bind %s <Return> {%s CornerChanged %i}",
                 this->CornerText[i]->GetWidgetName(), 
                 this->GetTclName(), i);
    this->Script("pack %s -side top -anchor w -padx 4 -expand yes",
                 this->CornerLabel[i]->GetWidgetName());
    this->Script("pack %s -side top -anchor w -padx 4 -pady 2 -expand yes -fill x",
                 this->CornerText[i]->GetWidgetName());
    }

  this->CornerLabel[0]->SetBalloonHelpString("Set the lower left corner annotation. The text will automatically scale to fit within the allocated space");

  this->CornerLabel[1]->SetBalloonHelpJustificationToRight();
  this->CornerLabel[1]->SetBalloonHelpString("Set the lower right corner annotation. The text will automatically scale to fit within the allocated space");

  this->CornerLabel[2]->SetBalloonHelpString("Set the upper left corner annotation. The text will automatically scale to fit within the allocated space");

  this->CornerLabel[3]->SetBalloonHelpJustificationToRight();
  this->CornerLabel[3]->SetBalloonHelpString("Set the upper right corner annotation. The text will automatically scale to fit within the allocated space");

}

void vtkKWCornerAnnotation::SetTextColor( float r, float g, float b )
{
  float *ff = this->CornerProp->GetProperty()->GetColor();
  if ( ff[0] == r && ff[1] == g && ff[2] == b )
    {
    return;
    }

  this->CornerColor->SetColor( r, g, b );
  this->CornerProp->GetProperty()->SetColor( r, g, b );
  this->View->Render();
  float color[3];
  color[0] = r;
  color[1] = g;
  color[2] = b;
  this->InvokeEvent( vtkKWEvent::AnnotationColorChangedEvent, color );
  this->InvokeEvent( vtkKWEvent::ViewAnnotationChangedEvent, 0 );
}

void vtkKWCornerAnnotation::OnDisplayCorner() 
{
  if (this->CornerButton->GetState())
    {
    this->View->AddComposite(this->CornerComposite);
    for (int i = 0; i < 4; i++)
      {
      this->CornerProp->SetText(i,this->GetCornerText(i));
      }
    this->View->Render();
    }
  else
    {
    this->View->RemoveComposite(this->CornerComposite);
    this->View->Render();
    }
  this->InvokeEvent( vtkKWEvent::ViewAnnotationChangedEvent, 0 );
}

int vtkKWCornerAnnotation::GetVisibility() 
{
  return this->CornerButton->GetState();
}

void vtkKWCornerAnnotation::SetVisibility(int state) 
{
  if (state == this->CornerButton->GetState())
    {
    return;
    }
  this->CornerButton->SetState(state);
  this->OnDisplayCorner();
}

void vtkKWCornerAnnotation::SetCornerText(const char *text, int corner) 
{
  this->CornerText[corner]->SetValue(text);
  this->CornerChanged(corner);
}

void vtkKWCornerAnnotation::CornerChanged(int i) 
{
  this->CornerProp->SetText(i,this->CornerText[i]->GetValue());
  if (this->CornerButton->GetState())
    {
    this->View->Render();
    }
  this->InvokeEvent( vtkKWEvent::ViewAnnotationChangedEvent, 0 );
}

// Description:
// Chaining method to serialize an object and its superclasses.
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

  os << indent << "CornerButton " << this->CornerButton->GetState() << endl;
  os << indent << "CornerColor ";
  this->CornerColor->Serialize(os,indent);
}

void vtkKWCornerAnnotation::SerializeToken(istream& is, 
                                           const char token[1024])
{
  int i;
  char tmp[1024];
  
  if (!strcmp(token,"CornerButton"))
    {
    is >> i;
    this->CornerButton->SetState(i);
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
  if (!strcmp(token,"CornerColor"))
    {
    this->CornerColor->Serialize(is);
    return;
    }

  vtkKWLabeledFrame::SerializeToken(is,token);
}

void vtkKWCornerAnnotation::SerializeRevision(ostream& os, vtkIndent indent)
{
  os << indent << "vtkKWCornerAnnotation ";
  this->ExtractRevision(os,"$Revision: 1.23 $");
  vtkKWLabeledFrame::SerializeRevision(os,indent);
}

char *vtkKWCornerAnnotation::GetCornerText(int i)
{
  return this->CornerText[i]->GetValue();
}

float *vtkKWCornerAnnotation::GetTextColor() 
{
  return this->CornerProp->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkKWCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerProp: " << this->GetCornerProp() << endl;
  os << indent << "View: " << this->GetView() << endl;
}
