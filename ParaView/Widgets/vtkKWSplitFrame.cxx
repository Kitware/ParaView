/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSplitFrame.cxx
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
#include "vtkKWSplitFrame.h"
#include "vtkObjectFactory.h"



vtkStandardNewMacro( vtkKWSplitFrame );




int vtkKWSplitFrameCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWSplitFrame::vtkKWSplitFrame()
{
  this->CommandFunction = vtkKWSplitFrameCommand;

  this->Frame1 = vtkKWWidget::New();
  this->Frame1->SetParent(this);
  this->Separator = vtkKWWidget::New();
  this->Separator->SetParent(this);
  this->Frame2 = vtkKWWidget::New();
  this->Frame2->SetParent(this);

  this->Frame1Size = 100;
  this->Frame2Size = 700;
  this->Frame1MinimumSize = 50;
  this->Frame2MinimumSize = 50;

  this->SeparatorSize = 6;

  this->Size = 806;

  this->Orientation = vtkKWSplitFrame::Horizontal;
}

//----------------------------------------------------------------------------
vtkKWSplitFrame::~vtkKWSplitFrame()
{
  this->Frame1->Delete();
  this->Frame1 = NULL;
  this->Separator->Delete();
  this->Separator = NULL;
  this->Frame2->Delete();
  this->Frame2 = NULL;
}


//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetOrientationToHorizontal()
{
  this->SetOrientation(vtkKWSplitFrame::Horizontal);
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetOrientationToVertical()
{
  this->SetOrientation(vtkKWSplitFrame::Vertical);
}


//----------------------------------------------------------------------------
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
               this->GetWidgetName(), this->GetTclName());


  // Setup the cursor to indication an action associatyed with the separator. 
#ifdef _WIN32
  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {  
    this->Script("%s configure -cursor size_we",
                 this->Separator->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -cursor size_ns",
                 this->Separator->GetWidgetName());
    }
#else
  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {  
    this->Script("%s configure -cursor sb_h_double_arrow",
                 this->Separator->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -cursor sb_v_double_arrow",
                 this->Separator->GetWidgetName());
    }
#endif
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::ConfigureCallback()
{
  int tmp;
  int size;

  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    this->Script( "winfo width %s", this->GetWidgetName());
    }
  else
    {
    this->Script( "winfo height %s", this->GetWidgetName());
    }
  size = vtkKWObject::GetIntegerResult(this->Application);   

  // First check to see if we have to make the window larger.
  tmp = this->Frame1MinimumSize + this->Frame2MinimumSize 
          + this->SeparatorSize;
  if (size < tmp)
    {
    this->Frame1Size = this->Frame1MinimumSize;
    this->Frame2Size = this->Frame2MinimumSize;
    this->Size = tmp;
    if (this->Orientation == vtkKWSplitFrame::Horizontal)
      {
      this->Script("%s configure -width %d", this->GetWidgetName(), this->Size);
      }
    else
      {
      this->Script("%s configure -height %d", this->GetWidgetName(), this->Size);
      }
    this->Update();
    return;
    }

  // Size of second frame
  this->Size = size;
  tmp = size - this->Frame1Size - this->SeparatorSize;

  // Do we have to adjust the first frame size?
  if (tmp < this->Frame2MinimumSize)
    {
    this->Frame2Size = this->Frame2MinimumSize;
    this->Frame1Size = size - this->Frame2Size - this->SeparatorSize;
    this->Update();
    return;
    }

  this->Frame2Size = tmp;
  this->Update();
}





//----------------------------------------------------------------------------
void vtkKWSplitFrame::DragCallback()
{
  int size, s, smin, tmp;
  int halfSepSize1, halfSepSize2;

  halfSepSize1 = this->SeparatorSize/2;
  halfSepSize2 = this->SeparatorSize-halfSepSize1;

  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    this->Script( "winfo width %s", this->GetWidgetName());
    }
  else
    {
    this->Script( "winfo height %s", this->GetWidgetName());
    }
  size = vtkKWObject::GetIntegerResult(this->Application); 
  this->Size = size;

  // get the relative position of the render in the window
  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    this->Script( "winfo rootx %s",this->GetWidgetName());
    }
  else
    {
    this->Script( "winfo rooty %s",this->GetWidgetName());
    } 
  smin = vtkKWObject::GetIntegerResult(this->Application);
  // Get the position of the mouse in the renderer.
  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    this->Script( "winfo pointerx %s", this->GetWidgetName());
    }
  else
    {
    this->Script( "winfo pointery %s", this->GetWidgetName());
    }
  s = vtkKWObject::GetIntegerResult(this->Application);
  // Relative position.
  s = s - smin;

  // Constraints on the window.
  tmp = this->Frame1MinimumSize + halfSepSize1;
  if (s < tmp)
    {
    s = tmp;
    }
  tmp = size - this->Frame2MinimumSize - halfSepSize2;
  if (s > tmp)
    {
    s = tmp;
    }

  this->Frame1Size = s-halfSepSize1;
  this->Frame2Size = size-s-halfSepSize2;
  this->Update();
}


//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame1MinimumSize(int minSize)
{
  if (this->Frame1MinimumSize == minSize)
    {
    return;
    }

  this->Frame1MinimumSize = minSize;

  if (this->Frame1Size < minSize)
    {
    this->Frame1Size = minSize;
    // Shrink frame2 if possible.
    this->Frame2Size = this->Size - this->Frame1Size - this->SeparatorSize;
    // Will the top frame actually grow because of this (configure).
    if (this->Frame2Size < this->Frame2MinimumSize)
      {
      this->Frame2Size = this->Frame2MinimumSize;
      this->Size = this->Frame1Size + this->Frame2Size + this->SeparatorSize;
      if (this->Orientation == vtkKWSplitFrame::Horizontal)
        {
        this->Script("%s configure -width %d", this->GetWidgetName(), this->Size);
        }
      else
        {
        this->Script("%s configure -height %d", this->GetWidgetName(), this->Size);
        }
      }
    this->Update();
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame2MinimumSize(int minSize)
{
  if (this->Frame2MinimumSize == minSize)
    {
    return;
    }

  this->Frame2MinimumSize = minSize;

  if (this->Frame2Size < minSize)
    {
    this->Frame2Size = minSize;
    // Shrink frame1 if possible.
    this->Frame1Size = this->Size - this->Frame1Size - this->SeparatorSize;
    // Will the top frame actually grow because of this (configure).
    if (this->Frame1Size < this->Frame1MinimumSize)
      {
      this->Frame1Size = this->Frame1MinimumSize;
      this->Size = this->Frame1Size + this->Frame2Size + this->SeparatorSize;
      if (this->Orientation == vtkKWSplitFrame::Horizontal)
        {
        this->Script("%s configure -width %d", this->GetWidgetName(), this->Size);
        }
      else
        {
        this->Script("%s configure -height %d", this->GetWidgetName(), this->Size);
        }
      }
    this->Update();
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame1Size(int size)
{
  if (this->Frame1Size == size)
    {
    return;
    }

  this->Frame1Size = size;
  this->Frame2Size = this->Size - this->Frame1Size - this->SeparatorSize;
  if (this->Frame2Size < this->Frame2MinimumSize)
    {
    this->Frame2Size = this->Frame2MinimumSize;
    this->Size = this->Frame1Size + this->Frame2Size + this->SeparatorSize;
    if (this->Orientation == vtkKWSplitFrame::Horizontal)
      {
      this->Script("%s configure -width %d", this->GetWidgetName(), this->Size);
      }
    else
      {
      this->Script("%s configure -height %d", this->GetWidgetName(), this->Size);
      }
    }
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetSeparatorSize(int size)
{

  if (this->SeparatorSize == size)
    {
    return;
    }

  this->SeparatorSize = size;
  // Update the frame sizes.
  this->SetFrame1Size(this->Frame1Size);
}


//----------------------------------------------------------------------------
void vtkKWSplitFrame::Update()
{
  if (this->Application == NULL)
    {
    return;
    }

  if (this->Orientation == vtkKWSplitFrame::Horizontal)  
    {
    this->Script("place %s -relx 0 -rely 0 -width %d -relheight 1.0",
                 this->Frame1->GetWidgetName(), this->Frame1Size);
    this->Script("place %s -x %d -rely 0 -width %d -relheight 1.0",
                 this->Separator->GetWidgetName(), this->Frame1Size,
                 this->SeparatorSize);
    this->Script("place %s -x %d -rely 0 -width %d -relheight 1.0",
                 this->Frame2->GetWidgetName(), 
                 this->Frame1Size+this->SeparatorSize, this->Frame2Size);
    }
  else
    {
    this->Script("place %s -relx 0 -rely 0 -relwidth 1.0 -height %d",
                 this->Frame1->GetWidgetName(), this->Frame1Size);
    this->Script("place %s -relx 0 -y %d -relwidth 1.0 -height %d",
                 this->Separator->GetWidgetName(), this->Frame1Size,
                 this->SeparatorSize);
    this->Script("place %s -relx 0 -y %d -height %d -relwidth 1.0",
                 this->Frame2->GetWidgetName(), 
                 this->Frame1Size+this->SeparatorSize, this->Frame2Size);
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame1MinimumSize: " << this->GetFrame1MinimumSize() 
     << endl;
  os << indent << "Frame1Size: " << this->GetFrame1Size() << endl;
  os << indent << "Frame2MinimumSize: " << this->GetFrame2MinimumSize() 
     << endl;
  os << indent << "Frame2Size: " << this->GetFrame2Size() << endl;
  os << indent << "SeparatorSize: " << this->GetSeparatorSize() << endl;

  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    os << indent << "Orientation: Horizontal\n";
    }
  else
    {
    os << indent << "Orientation: Vertical\n";
    }
}
