/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSegmentedProgressGauge.cxx
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
#include "vtkKWSegmentedProgressGauge.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKWSegmentedProgressGauge);
vtkCxxRevisionMacro(vtkKWSegmentedProgressGauge, "1.1");

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
  
  // must set the application
  if (this->Application)
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
    this->Script("%s create rectangle %d 0 %d %d -outline black -fill black -tags bar%d",
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
      this->Script("%s itemconfigure bar%d -fill $color%d -outline {}",
                   this->ProgressCanvas->GetWidgetName(), i, i);
      }
    else
      {
      this->Script("%s itemconfigure bar%d -fill black -outline black",
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
