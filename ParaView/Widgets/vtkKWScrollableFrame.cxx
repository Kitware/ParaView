/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWScrollableFrame.cxx
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
#include "vtkKWScrollableFrame.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWScrollableFrame );

vtkKWScrollableFrame::vtkKWScrollableFrame()
{

  this->Canvas = vtkKWWidget::New();
  this->ScrollBar = vtkKWWidget::New();
  this->Frame = vtkKWWidget::New();
  this->FrameId = 0;
}

vtkKWScrollableFrame::~vtkKWScrollableFrame()
{
  this->Canvas->Delete();
  this->ScrollBar->Delete();
  this->Frame->Delete();
  delete[] this->FrameId;
}


void vtkKWScrollableFrame::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("ScrollableFrame already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 2 -relief flat", wname);

  this->Canvas->SetParent(this);
  this->Canvas->Create(this->Application, "canvas", " -height 800"); 

  ostrstream command;
  this->ScrollBar->SetParent(this);
  command << "-command \"" <<  this->Canvas->GetWidgetName()
	  << " yview\"" << ends;
  char* commandStr = command.str();
  this->ScrollBar->Create(this->Application, "scrollbar", commandStr);
  delete[] commandStr;

  this->Script("%s configure -yscrollcommand \"%s set\"", 
	       this->Canvas->GetWidgetName(),
	       this->ScrollBar->GetWidgetName());

  this->Frame->SetParent(this->Canvas);
  this->Frame->Create(this->Application, "frame", ""); 

  this->Script("%s configure -yscrollcommand \"%s set\"", 
	       this->Canvas->GetWidgetName(),
	       this->ScrollBar->GetWidgetName());
  this->Script("%s create window 0 0 -anchor nw -window %s",
	       this->Canvas->GetWidgetName(),
	       this->Frame->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  this->FrameId = new char[strlen(result)+1];
  strcpy(this->FrameId,result);

  this->Script("bind %s <Configure> {%s ResizeFrame}",
	       this->Frame->GetWidgetName(),
	       this->GetTclName());
  this->Script("bind %s <Configure> {%s ResizeCanvas}",
	       this->Canvas->GetWidgetName(),
	       this->GetTclName());

  this->Script("pack %s -fill both -expand t -side left", this->Canvas->GetWidgetName());
}

void vtkKWScrollableFrame::CalculateBBox(vtkKWWidget* canvas, char* name, 
					 int bbox[4])
{
  char *result;

  // Get the bounding box for the name. We may need to highlight it.
  this->Script("%s bbox %s", canvas->GetWidgetName(), name);
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);

}

void vtkKWScrollableFrame::ResizeFrame()
{
  this->Script("winfo width %s", this->Frame->GetWidgetName());
  //int widthFrame = this->GetIntegerResult(this->Application);

  int bbox[4];
  this->CalculateBBox(this->Canvas, "all", bbox);
  this->Script("%s configure  -scrollregion \"%d %d %d %d\" "
	       "-yscrollincrement 0.1i", this->Canvas->GetWidgetName(), 
	       bbox[0], bbox[1], bbox[2], bbox[3]);
}

void vtkKWScrollableFrame::ResizeCanvas()
{
  this->Script("winfo width %s", this->Frame->GetWidgetName());
  //int widthFrame = this->GetIntegerResult(this->Application);

  int bbox[4], heightCanvas, heightFrame;

  this->CalculateBBox(this->Canvas, "all", bbox);
  heightFrame = bbox[3] - bbox[1];
  this->Script("winfo height %s", this->Canvas->GetWidgetName());
  heightCanvas = this->GetIntegerResult(this->Application);
  if (heightFrame > heightCanvas)
    {
    this->Script("pack forget %s", this->Canvas->GetWidgetName());
    this->Script("pack %s -fill both -expand t -side left", this->Canvas->GetWidgetName());
    this->Script("pack %s -fill both -expand t -side right", 
		 this->ScrollBar->GetWidgetName());
    }
  else
    {
    this->Script("pack forget %s", this->ScrollBar->GetWidgetName());
    }
  this->Script("%s itemconfigure %s -width [winfo width %s]",
	       this->Canvas->GetWidgetName(),
	       this->FrameId,
	       this->Canvas->GetWidgetName());

}
