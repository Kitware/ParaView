/*=========================================================================

  Module:    vtkKWSplitFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWSplitFrame.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWSplitFrame );
vtkCxxRevisionMacro(vtkKWSplitFrame, "1.23");

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

  this->Frame1Visibility = 1;
  this->Frame2Visibility = 1;

  this->SeparatorSize = 4;
  this->SeparatorMargin = 2;

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
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(
        app, "frame", "-bd 0 -relief flat -width 200 -height 100"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Frame1->Create(app,"frame","-bd 0 -relief flat");
  this->Separator->Create(app,"frame","-bd 2 -relief raised");
  this->Frame2->Create(app,"frame","-bd 0 -relief flat");
  
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

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::ConfigureCallback()
{
  int tmp;
  int size;

  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    size = atoi(this->Script( "winfo width %s", this->GetWidgetName()));
    }
  else
    {
    size = atoi(this->Script( "winfo height %s", this->GetWidgetName()));
    }

  // If size == 1 then the widget has not been packed, it will be later
  // and the Configure event will bring us back here with the correct size

  if (size == 1)
    {
    return;
    }

  // First check to see if we have to make the window larger.
  tmp = this->Frame1MinimumSize + this->Frame2MinimumSize 
          + this->GetTotalSeparatorSize();
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
  tmp = size - this->Frame1Size - this->GetTotalSeparatorSize();

  // Do we have to adjust the first frame size?
  if (tmp < this->Frame2MinimumSize)
    {
    this->Frame2Size = this->Frame2MinimumSize;
    this->Frame1Size = size - this->Frame2Size - this->GetTotalSeparatorSize();
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

  halfSepSize1 = this->GetTotalSeparatorSize()/2;
  halfSepSize2 = this->GetTotalSeparatorSize()-halfSepSize1;

  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    size = atoi(this->Script( "winfo width %s", this->GetWidgetName()));
    }
  else
    {
    size = atoi(this->Script( "winfo height %s", this->GetWidgetName()));
    }
  this->Size = size;

  // get the relative position of the render in the window
  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    smin = atoi(this->Script( "winfo rootx %s",this->GetWidgetName()));
    }
  else
    {
    smin = atoi(this->Script( "winfo rooty %s",this->GetWidgetName()));
    } 

  // Get the position of the mouse in the renderer.
  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    s = atoi(this->Script( "winfo pointerx %s", this->GetWidgetName()));
    }
  else
    {
    s = atoi(this->Script( "winfo pointery %s", this->GetWidgetName()));
    }

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
    this->Frame2Size = this->Size - this->Frame1Size - this->GetTotalSeparatorSize();
    // Will the top frame actually grow because of this (configure).
    if (this->Frame2Size < this->Frame2MinimumSize)
      {
      this->Frame2Size = this->Frame2MinimumSize;
      this->Size = this->Frame1Size + this->Frame2Size + this->GetTotalSeparatorSize();
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
    this->Frame1Size = this->Size - this->Frame1Size - this->GetTotalSeparatorSize();
    // Will the top frame actually grow because of this (configure).
    if (this->Frame1Size < this->Frame1MinimumSize)
      {
      this->Frame1Size = this->Frame1MinimumSize;
      this->Size = this->Frame1Size + this->Frame2Size + this->GetTotalSeparatorSize();
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
  this->Frame2Size = this->Size - this->Frame1Size - this->GetTotalSeparatorSize();
  if (this->Frame2Size < this->Frame2MinimumSize)
    {
    this->Frame2Size = this->Frame2MinimumSize;
    this->Size = this->Frame1Size + this->Frame2Size + this->GetTotalSeparatorSize();
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
void vtkKWSplitFrame::SetSeparatorMargin(int size)
{
  if (this->SeparatorMargin == size)
    {
    return;
    }

  this->SeparatorMargin = size;
  // Update the frame sizes.
  this->SetFrame1Size(this->Frame1Size);
}

//----------------------------------------------------------------------------
int vtkKWSplitFrame::GetTotalSeparatorSize()
{
  return this->SeparatorMargin * 2 + this->SeparatorSize;
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame1Visibility(int flag)
{
  if (this->Frame1Visibility == flag)
    {
    return;
    }

  this->Frame1Visibility = flag;
  this->Modified();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame2Visibility(int flag)
{
  if (this->Frame2Visibility == flag)
    {
    return;
    }

  this->Frame2Visibility = flag;
  this->Modified();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::Update()
{
  if (!this->IsCreated())
    {
    return;
    }

  int total_separator_size, separator_size, separator_margin;
  int frame1_size, frame2_size;

  if (this->Frame1Visibility && this->Frame2Visibility)
    {
    separator_size = this->SeparatorSize;
    separator_margin = this->SeparatorMargin;
    total_separator_size = this->GetTotalSeparatorSize();
    frame1_size = this->Frame1Size;
    frame2_size = this->Frame2Size;
    }
  else
    {
    total_separator_size = separator_size = separator_margin = 0;
    if (this->Frame1Visibility)
      {
      frame1_size = this->Size;
      frame2_size = 0;
      }
    else if (this->Frame2Visibility)
      {
      frame1_size = 0;
      frame2_size = this->Size;
      }
    else
      {
      frame1_size = frame2_size = 0;
      }
    }

  if (this->Frame1Visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::Horizontal)  
      {
      this->Script("place %s -relx 0 -rely 0 -width %d -relheight 1.0",
                   this->Frame1->GetWidgetName(), frame1_size);
      }
    else
      {
      this->Script("place %s -relx 0 -rely 0 -relwidth 1.0 -height %d",
                   this->Frame1->GetWidgetName(), frame1_size);
      }
    }
  else
    {
    this->Script("place forget %s", this->Frame1->GetWidgetName());
    }

  if (this->Frame1Visibility && this->Frame2Visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::Horizontal)  
      {
      this->Script("place %s -x %d -rely 0 -width %d -relheight 1.0",
                   this->Separator->GetWidgetName(), 
                   frame1_size + separator_margin,
                   separator_size);
      }
    else
      {
      this->Script("place %s -relx 0 -y %d -relwidth 1.0 -height %d",
                   this->Separator->GetWidgetName(), 
                   frame1_size + separator_margin,
                   separator_size);
      }
    }
  else
    {
    this->Script("place forget %s", this->Separator->GetWidgetName());
    }

  if (this->Frame2Visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::Horizontal)  
      {
      this->Script("place %s -x %d -rely 0 -width %d -relheight 1.0",
                   this->Frame2->GetWidgetName(), 
                   frame1_size + total_separator_size, frame2_size);
      }
    else
      {
      this->Script("place %s -relx 0 -y %d -height %d -relwidth 1.0",
                   this->Frame2->GetWidgetName(), 
                   frame1_size + total_separator_size, frame2_size);
      }
    }
  else
    {
    this->Script("place forget %s", this->Frame2->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame1);
  this->PropagateEnableState(this->Separator);
  this->PropagateEnableState(this->Frame2);
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame1MinimumSize: " << this->GetFrame1MinimumSize() 
     << endl;
  os << indent << "Frame1Size: " << this->GetFrame1Size() << endl;
  os << indent << "Frame1Visibility: " << (this->Frame1Visibility ? "On" : "Off") << endl;
  os << indent << "Frame2MinimumSize: " << this->GetFrame2MinimumSize() 
     << endl;
  os << indent << "Frame2Size: " << this->GetFrame2Size() << endl;
  os << indent << "Frame2Visibility: " << (this->Frame2Visibility ? "On" : "Off") << endl;
  os << indent << "SeparatorSize: " << this->GetSeparatorSize() << endl;
  os << indent << "SeparatorMargin: " << this->GetSeparatorMargin() << endl;

  if (this->Orientation == vtkKWSplitFrame::Horizontal)
    {
    os << indent << "Orientation: Horizontal\n";
    }
  else
    {
    os << indent << "Orientation: Vertical\n";
    }
}

