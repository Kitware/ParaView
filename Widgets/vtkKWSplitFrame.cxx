/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSplitFrame.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWSplitFrame.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWSplitFrame* vtkKWSplitFrame::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWSplitFrame");
  if(ret)
    {
    return (vtkKWSplitFrame*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWSplitFrame;
}




int vtkKWSplitFrameCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWSplitFrame::vtkKWSplitFrame()
{
  this->CommandFunction = vtkKWSplitFrameCommand;

  this->Frame1 = vtkKWWidget::New();
  this->Frame1->SetParent(this);
  this->Separator = vtkKWWidget::New();
  this->Separator->SetParent(this);
  this->Frame2 = vtkKWWidget::New();
  this->Frame2->SetParent(this);

  this->Frame1Width = 100;
  this->Frame2Width = 700;
  this->Frame1MinimumWidth = 50;
  this->Frame2MinimumWidth = 50;

  this->SeparatorWidth = 6;

  this->Width = 806;
}

vtkKWSplitFrame::~vtkKWSplitFrame()
{
  this->Frame1->Delete();
  this->Frame1 = NULL;
  this->Separator->Delete();
  this->Separator = NULL;
  this->Frame2->Delete();
  this->Frame2 = NULL;
}


void vtkKWSplitFrame::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("SplitFrame already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat -width 200 -height 100",
               wname);

  this->Frame1->Create(app,"frame","-borderwidth 0 -relief flat");
  this->Separator->Create(app,"frame","-borderwidth 2 -relief raised");
  this->Frame2->Create(app,"frame","-borderwidth 0 -relief flat");
  
  this->Update();

  this->Script("bind %s <B1-Motion> {%s DragCallback}",
               this->Separator->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Configure> {%s ConfigureCallback}",
               this->Separator->GetWidgetName(), this->GetTclName());


  // Setup the cursor to indication an action associatyed with the separator. 
#ifdef _WIN32  
  this->Script("%s configure -cursor size_we",
               this->Separator->GetWidgetName());
#else
  this->Script("%s configure -cursor sb_h_double_arrow",
               this->Separator->GetWidgetName());
#endif
}

void vtkKWSplitFrame::ConfigureCallback()
{
  int tmp;
  int width;

  this->Script( "winfo width %s", this->GetWidgetName());
  width = vtkKWObject::GetIntegerResult(this->Application);   

  // First check to see if we have to make the window larger.
  tmp = this->Frame1MinimumWidth + this->Frame2MinimumWidth 
          + this->SeparatorWidth;
  if (width < tmp)
    {
    this->Frame1Width = this->Frame1MinimumWidth;
    this->Frame2Width = this->Frame2MinimumWidth;
    this->Width = tmp;
    this->Script("%s configure -width %d", this->GetWidgetName(), this->Width);
    this->Update();
    return;
    }

  // Width of second frame
  this->Width = width;
  tmp = width - this->Frame1Width - this->SeparatorWidth;

  // Do we have to adjust the first frame width?
  if (tmp < this->Frame2MinimumWidth)
    {
    this->Frame2Width = this->Frame2MinimumWidth;
    this->Frame1Width = width - this->Frame2Width - this->SeparatorWidth;
    this->Update();
    return;
    }

  this->Frame2Width = tmp;
  this->Update();
}





void vtkKWSplitFrame::DragCallback()
{
  int width, x, xmin, tmp;
  int h1, h2;

  h1 = this->SeparatorWidth/2;
  h2 = this->SeparatorWidth-h1;

  this->Script( "winfo width %s", this->GetWidgetName());
  width = vtkKWObject::GetIntegerResult(this->Application); 

  this->Width = width;

  // get the relative position of the render in the window
  this->Script( "winfo rootx %s",this->GetWidgetName());
  xmin = vtkKWObject::GetIntegerResult(this->Application);
  // Get the position of the mouse in the renderer.
  this->Script( "winfo pointerx %s", this->GetWidgetName());
  x = vtkKWObject::GetIntegerResult(this->Application);
  // Relative position.
  x = x - xmin;

  // Constraints on the window.
  tmp = this->Frame1MinimumWidth + h1;
  if (x < tmp)
    {
    x = tmp;
    }
  tmp = width - this->Frame2MinimumWidth - h2;
  if (x > tmp)
    {
    x = tmp;
    }

  this->Frame1Width = x-h1;
  this->Frame2Width = width-x-h2;
  this->Update();
}


void vtkKWSplitFrame::SetFrame1MinimumWidth(int minWidth)
{
  if (this->Frame1MinimumWidth == minWidth)
    {
    return;
    }

  this->Frame1MinimumWidth = minWidth;

  if (this->Frame1Width < minWidth)
    {
    this->Frame1Width = minWidth;
    // Shrink frame2 if possible.
    this->Frame2Width = this->Width - this->Frame1Width - this->SeparatorWidth;
    // Will the top frame actually grow because of this (configure).
    if (this->Frame2Width < this->Frame2MinimumWidth)
      {
      this->Frame2Width = this->Frame2MinimumWidth;
      this->Width = this->Frame1Width + this->Frame2Width + this->SeparatorWidth;
      this->Script("%s configure -width %d", this->GetWidgetName(), this->Width);
      }
    this->Update();
    }
}

void vtkKWSplitFrame::SetFrame2MinimumWidth(int minWidth)
{
  if (this->Frame2MinimumWidth == minWidth)
    {
    return;
    }

  this->Frame2MinimumWidth = minWidth;

  if (this->Frame2Width < minWidth)
    {
    this->Frame2Width = minWidth;
    // Shrink frame1 if possible.
    this->Frame1Width = this->Width - this->Frame1Width - this->SeparatorWidth;
    // Will the top frame actually grow because of this (configure).
    if (this->Frame1Width < this->Frame1MinimumWidth)
      {
      this->Frame1Width = this->Frame1MinimumWidth;
      this->Width = this->Frame1Width + this->Frame2Width + this->SeparatorWidth;
      this->Script("%s configure -width %d", this->GetWidgetName(), this->Width);
      }
    this->Update();
    }
}

void vtkKWSplitFrame::SetFrame1Width(int width)
{
  if (this->Frame1Width == width)
    {
    return;
    }

  this->Frame1Width = width;
  this->Frame2Width = this->Width - this->Frame1Width - this->SeparatorWidth;
  if (this->Frame2Width < this->Frame2MinimumWidth)
    {
    this->Frame2Width = this->Frame2MinimumWidth;
    this->Width = this->Frame1Width + this->Frame2Width + this->SeparatorWidth;
    this->Script("%s configure -width %d", this->GetWidgetName(), this->Width);
    }
  this->Update();
}

void vtkKWSplitFrame::SetSeparatorWidth(int width)
{

  if (this->SeparatorWidth == width)
    {
    return;
    }

  this->SeparatorWidth = width;
  // Update the frame sizes.
  this->SetFrame1Width(this->Frame1Width);
}


void vtkKWSplitFrame::Update()
{
  this->Script("place %s -relx 0 -rely 0 -width %d -relheight 1.0",
               this->Frame1->GetWidgetName(), this->Frame1Width);
  this->Script("place %s -x %d -rely 0 -width %d -relheight 1.0",
               this->Separator->GetWidgetName(), this->Frame1Width,
               this->SeparatorWidth);
  this->Script("place %s -x %d -rely 0 -width %d -relheight 1.0",
               this->Frame2->GetWidgetName(), 
               this->Frame1Width+this->SeparatorWidth, this->Frame2Width);
}