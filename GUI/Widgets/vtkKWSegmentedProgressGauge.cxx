/*=========================================================================

  Module:    vtkKWSegmentedProgressGauge.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWCanvas.h"
#include "vtkKWFrame.h"
#include "vtkKWSegmentedProgressGauge.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKWSegmentedProgressGauge);
vtkCxxRevisionMacro(vtkKWSegmentedProgressGauge, "1.9");

vtkKWSegmentedProgressGauge::vtkKWSegmentedProgressGauge()
{
  this->ProgressFrame = vtkKWFrame::New();
  this->ProgressFrame->SetParent(this);
  this->ProgressCanvas = vtkKWCanvas::New();
  this->ProgressCanvas->SetParent(this->ProgressFrame);
  
  this->NumberOfSegments = 3;
  this->Width = 100;
  this->Height = 7;
  
  this->Segment = 0;
  this->Value = 0;
  
  for ( int i = 0; i < 10; i++ )
    {
    this->SegmentColor[i][0] = 0;
    this->SegmentColor[i][1] = static_cast<float>(i)/9;
    this->SegmentColor[i][2] = 1.0 - static_cast<float>(i)/9;
    }
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
  
  this->ProgressFrame->Create(app, "-bd 1 -relief sunken");
  
  this->ProgressCanvas->Create(app, "");
  this->Script("%s configure -borderwidth 0 -highlightthickness 0 -background #008 -width %d -height %d",
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
      char colorString[32];
      sprintf( colorString, "#%02x%02x%02x",
               static_cast<int>(this->SegmentColor[i][0]*255 + 0.5),
               static_cast<int>(this->SegmentColor[i][1]*255 + 0.5),
               static_cast<int>(this->SegmentColor[i][2]*255 + 0.5) );
               
      this->Script("%s itemconfigure bar%d -fill %s",
                   this->ProgressCanvas->GetWidgetName(), i, colorString);
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

void vtkKWSegmentedProgressGauge::SetNumberOfSegments(int number)
{
  if (number < 1 || number > 10)
    {
    return;
    }

  int prevSegments = this->NumberOfSegments;
  this->NumberOfSegments = number;
  
  if (!this->IsCreated())
    {
    return;
    }
  
  int i;
  for (i = 0; i < prevSegments; i++)
    {
    this->Script("%s delete bar%d",
                 this->ProgressCanvas->GetWidgetName(), i);
    }

  for (i = 0; i < this->NumberOfSegments; i++)
    {
    this->Script("%s create rectangle %d 0 %d %d -fill #008 -tags bar%d",
                 this->ProgressCanvas->GetWidgetName(),
                 (int)(i*this->Width/(float)this->NumberOfSegments),
                 (int)((i+1)*(this->Width/(float)this->NumberOfSegments)),
                 this->Height, i);
    }
}

void vtkKWSegmentedProgressGauge::SetSegmentColor( int index, float r, float g, float b )
{
  if ( index < 0 || index > 9 )
    {
    vtkErrorMacro("Invalid index in SetSegmentColor: " << index );
    return;
    }
  
  this->SegmentColor[index][0] = r;
  this->SegmentColor[index][1] = g;
  this->SegmentColor[index][2] = b;
  
  this->Modified();
}

void vtkKWSegmentedProgressGauge::GetSegmentColor( int index, float color[3] )
{
  if ( index < 0 || index > 9 )
    {
    vtkErrorMacro("Invalid index in SetSegmentColor: " << index );
    return;
    }
  
  color[0] = this->SegmentColor[index][0];
  color[1] = this->SegmentColor[index][1];
  color[2] = this->SegmentColor[index][2];
}

void vtkKWSegmentedProgressGauge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "NumberOfSegments: " << this->NumberOfSegments << endl;
  os << indent << "Width: " << this->Width << endl;
  os << indent << "Height: " << this->Height << endl;
}

