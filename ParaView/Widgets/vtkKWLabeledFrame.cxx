/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledFrame.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWImageLabel.h"
#include "vtkKWIcon.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabeledFrame );




int vtkKWLabeledFrameCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWLabeledFrame::vtkKWLabeledFrame()
{
  this->CommandFunction = vtkKWLabeledFrameCommand;

  this->Border = vtkKWWidget::New();
  this->Border->SetParent(this);
  this->Groove = vtkKWWidget::New();
  this->Groove->SetParent(this);
  this->Border2 = vtkKWWidget::New();
  this->Border2->SetParent(this->Groove);
  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this->Groove);
  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this);
  this->Icon = vtkKWImageLabel::New();
  this->Icon->SetParent(this);
  this->IconData = vtkKWIcon::New();
  this->IconData->SetImageData(vtkKWIcon::ICON_SHRINK);
  this->Displayed = 1;
}

vtkKWLabeledFrame::~vtkKWLabeledFrame()
{
  this->Icon->Delete();
  this->IconData->Delete();
  this->Label->Delete();
  this->Frame->Delete();
  this->Border->Delete();
  this->Border2->Delete();
  this->Groove->Delete();
}

void vtkKWLabeledFrame::SetLabel(const char *text)
{
  this->Script("%s configure -text {%s}",
               this->Label->GetWidgetName(),text);  
}

void vtkKWLabeledFrame::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("LabeledFrame already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat",wname);

  this->Border->Create(app,"frame","-height 10 -borderwidth 0 -relief flat");
  this->Label->Create(app,"label","");
  this->Groove->Create(app,"frame","-borderwidth 2 -relief groove");
  this->Border2->Create(app,"frame","-height 10 -borderwidth 0 -relief flat");
  this->Frame->Create(app,"frame","-borderwidth 0 -relief flat");
  this->Icon->Create(app,"");
  this->Icon->SetImageData(this->IconData);
  this->Icon->SetBalloonHelpString("Shrink or expand the frame");
  
  this->Script("pack %s -fill x -side top", this->Border->GetWidgetName());
  this->Script("pack %s -fill x -side top", this->Groove->GetWidgetName());
  this->Script("pack %s -fill x -side top", this->Border2->GetWidgetName());
  this->Script("pack %s -fill both -expand yes",this->Frame->GetWidgetName());
  this->Script("place %s -relx 0 -x 5 -y 0 -anchor nw",
               this->Label->GetWidgetName());

  this->Script("raise %s", this->Label->GetWidgetName());

  if ( vtkKWLabeledFrame::AllowShowHide )
    {
    this->Script("place %s -relx 1 -x -5 -y 2 -anchor ne",
		 this->Icon->GetWidgetName());    
    this->Script("bind %s <ButtonRelease-1> { %s ShowHideFrame }",
		 this->Icon->GetWidgetName(),
		 this->GetTclName());
    this->Script("raise %s", this->Icon->GetWidgetName());
    }
}

void vtkKWLabeledFrame::ShowHideFrame()
{
  if ( this->Displayed )
    {
    this->Script("pack forget %s", this->Frame->GetWidgetName());
    this->Displayed = 0;
    this->IconData->SetImageData(vtkKWIcon::ICON_EXPAND);
    }
  else
    {
    this->Script("pack %s -fill both -expand yes",
		 this->Frame->GetWidgetName());
    this->Displayed = 1;
    this->IconData->SetImageData(vtkKWIcon::ICON_SHRINK);
   }
  this->Icon->SetImageData(this->IconData);
}

int vtkKWLabeledFrame::AllowShowHide = 0;

void vtkKWLabeledFrame::AllowShowHideOn() 
{ 
  vtkKWLabeledFrame::AllowShowHide = 1; 
}
void vtkKWLabeledFrame::AllowShowHideOff() 
{ 
  vtkKWLabeledFrame::AllowShowHide = 0; 
}
