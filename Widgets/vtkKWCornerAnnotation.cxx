/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCornerAnnotation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWCornerAnnotation.h"
#include "vtkKWWindow.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWCornerAnnotation* vtkKWCornerAnnotation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWCornerAnnotation");
  if(ret)
    {
    return (vtkKWCornerAnnotation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWCornerAnnotation;
}




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
    this->CornerMapper[i] = vtkTextMapper::New();
    this->CornerMapper[i]->SetFontSize(15);  
    this->CornerMapper[i]->ShadowOff();  
    this->CornerProp[i] = vtkScaledTextActor::New();
    this->CornerProp[i]->SetMaximumLineHeight(0.25);
    this->CornerProp[i]->SetWidth(0.47);
    this->CornerProp[i]->SetHeight(0.25);
    this->CornerProp[i]->SetMapper(this->CornerMapper[i]);
    this->CornerComposite[i] = vtkKWGenericComposite::New();
    this->CornerComposite[i]->SetProp(this->CornerProp[i]);
    }

  this->CornerProp[0]->GetPositionCoordinate()->SetValue(0.01,0.02);
  this->CornerMapper[0]->SetJustificationToLeft();
  this->CornerMapper[0]->SetVerticalJustificationToBottom();

  this->CornerProp[1]->GetPositionCoordinate()->SetValue(0.52,0.02);
  this->CornerMapper[1]->SetJustificationToRight();
  this->CornerMapper[1]->SetVerticalJustificationToBottom();
  
  this->CornerProp[2]->GetPositionCoordinate()->SetValue(0.01,0.73);
  this->CornerMapper[2]->SetJustificationToLeft();
  this->CornerMapper[2]->SetVerticalJustificationToTop();
  
  this->CornerProp[3]->GetPositionCoordinate()->SetValue(0.52,0.73);
  this->CornerMapper[3]->SetJustificationToRight();
  this->CornerMapper[3]->SetVerticalJustificationToTop();
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
    this->CornerComposite[i]->Delete();
    this->CornerProp[i]->Delete();
    this->CornerMapper[i]->Delete();
    this->CornerLabel[i]->Delete();
    this->CornerText[i]->Delete();
    }
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
    this->View->RemoveComposite(this->CornerComposite[0]);
    this->View->RemoveComposite(this->CornerComposite[1]);
    this->View->RemoveComposite(this->CornerComposite[2]);
    this->View->RemoveComposite(this->CornerComposite[3]);
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
  this->CornerButton->SetCommand(this, "OnDisplayCorner");
  this->Script("pack %s -side left -padx 2 -pady 4 -anchor nw",
               this->CornerButton->GetWidgetName());
  this->CornerColor->Create( app, "" );
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
  
}

void vtkKWCornerAnnotation::SetTextColor( float r, float g, float b )
{
  this->CornerProp[0]->GetProperty()->SetColor( r, g, b );
  this->CornerProp[1]->GetProperty()->SetColor( r, g, b );
  this->CornerProp[2]->GetProperty()->SetColor( r, g, b );
  this->CornerProp[3]->GetProperty()->SetColor( r, g, b );
  this->View->Render();
}

void vtkKWCornerAnnotation::OnDisplayCorner() 
{
  if (this->CornerButton->GetState())
    {
    for (int i = 0; i < 4; i++)
      {
      this->View->AddComposite(this->CornerComposite[i]);
      this->CornerMapper[i]->SetInput(this->CornerText[i]->GetValue());
      }
    this->View->Render();
    }
  else
    {
    for (int i = 0; i < 4; i++)
      {
      this->View->RemoveComposite(this->CornerComposite[i]);
      }
    this->View->Render();
    }
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
  this->CornerMapper[i]->SetInput(this->CornerText[i]->GetValue());
  if (this->CornerButton->GetState())
    {
    this->View->Render();
    }
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
  vtkKWSerializer::WriteSafeString(os, this->CornerText[0]->GetValue());
  os << endl;
  os << indent << "CornerText2 ";
  vtkKWSerializer::WriteSafeString(os, this->CornerText[0]->GetValue());
  os << endl;
  os << indent << "CornerText3 ";
  vtkKWSerializer::WriteSafeString(os, this->CornerText[0]->GetValue());
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
  this->ExtractRevision(os,"$Revision: 1.3 $");
  vtkKWLabeledFrame::SerializeRevision(os,indent);
}
