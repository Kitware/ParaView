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
#include "vtkKWSegmentedProgressGauge.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKWSegmentedProgressGauge);
vtkCxxRevisionMacro(vtkKWSegmentedProgressGauge, "1.4");

vtkKWSegmentedProgressGauge::vtkKWSegmentedProgressGauge()
{
  this->ProgressFrame = vtkKWWidget::New();
  this->ProgressFrame->SetParent(this);
  this->ProgressCanvas = vtkKWWidget::New();
  this->ProgressCanvas->SetParent(this->ProgressFrame);
  
  this->NumberOfSegments = 3;
  this->Width = 100;
  this->Height = 7;
  
  this->Segment = 0;
  this->Value = 0;
}

vtkKWSegmentedProgressGauge::~vtkKWSegmentedProgressGauge()
{
  this->ProgressFrame->Delete();
  this->ProgressCanvas->Delete();
}

void vtkKWSegmentedProgressGauge::Create(vtkKWApplication *app,
                                         const char *args)
{
  const char *wname;
  
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Segmented progress gauge already created");
    return;
    }
  
  this->SetApplication(app);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s %s", wname, args);
  
  this->ProgressFrame->Create(app, "frame", "-bd 1 -relief sunken");
  
  this->ProgressCanvas->Create(app, "canvas", "");
  this->Script("%s configure -borderwidth 0 -highlightthickness 0 -width %d -height %d",
               this->ProgressCanvas->GetWidgetName(),
               this->Width, this->Height);
  
  this->Script("pack %s -expand yes", this->ProgressCanvas->GetWidgetName());
  this->Script("pack %s -expand yes", this->ProgressFrame->GetWidgetName());

  int i;
  for (i = 0; i < this->NumberOfSegments; i++)
    {
    this->Script("%s create rectangle %d 0 %d %d -fill #008 -tags bar%d",
                 this->ProgressCanvas->GetWidgetName(),
                 (int)(i*this->Width/(float)this->NumberOfSegments),
                 (int)((i+1)*(this->Width/(float)this->NumberOfSegments)),
                 this->Height, i);
    }
  
  switch (this->NumberOfSegments)
    {
    case 1:
      this->Script("set color0 green1");
      break;
    case 2:
      this->Script("set color0 red1; set color1 green1");
      break;
    case 3:
      this->Script("set color0 red1; set color1 yellow1; set color2 green1");
      break;
    case 4:
      this->Script("set color0 red1; set color1 orange1; set color2 yellow1; set color3 green1");
      break;
    }

  // Update enable state

  this->UpdateEnableState();
}

void vtkKWSegmentedProgressGauge::SetValue(int segment, int value)
{
  this->Segment = segment;
  if (this->Segment > this->NumberOfSegments - 1)
    {
    this->Segment = this->NumberOfSegments - 1;
    }
  else if (this->Segment < 0)
    {
    this->Segment = 0;
    }
  
  this->Value = value;
  if (this->Value > 100)
    {
    this->Value = 100;
    }
  else if (this->Value < 0)
    {
    this->Value = 0;
    }
  
  int i;
  for (i = 0; i < this->NumberOfSegments; i++)
    {
    if (i <= this->Segment)
      {
      this->Script("%s itemconfigure bar%d -fill $color%d",
                   this->ProgressCanvas->GetWidgetName(), i, i);
      }
    else
      {
      this->Script("%s itemconfigure bar%d -fill #008",
                   this->ProgressCanvas->GetWidgetName(), i);
      }
    
    if (i == this->Segment)
      {
      this->Script("%s coords bar%d %d 0 %d %d",
                   this->ProgressCanvas->GetWidgetName(), i,
                   (int)(i*this->Width/(float)this->NumberOfSegments),
                   (int)(this->Width/(float)this->NumberOfSegments *
                   (i + 0.01*this->Value)),
                   this->Height);
      }
    else
      {
      this->Script("%s coords bar%d %d 0 %d %d",
                   this->ProgressCanvas->GetWidgetName(), i,
                   (int)(i*this->Width/(float)this->NumberOfSegments),
                   (int)((i+1)*this->Width/(float)this->NumberOfSegments),
                   this->Height);
      }
    }
  this->Script("update idletasks");
}

void vtkKWSegmentedProgressGauge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "NumberOfSegments: " << this->NumberOfSegments << endl;
  os << indent << "Width: " << this->Width << endl;
  os << indent << "Height: " << this->Height << endl;
}

