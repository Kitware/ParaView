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
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWSplitFrame );
vtkCxxRevisionMacro(vtkKWSplitFrame, "1.36");

//----------------------------------------------------------------------------
vtkKWSplitFrame::vtkKWSplitFrame()
{
  this->Frame1 = vtkKWFrame::New();
  this->Separator = vtkKWFrame::New();
  this->Frame2 = vtkKWFrame::New();

  this->Frame1Size = 250;
  this->Frame2Size = 250;

  this->Frame1MinimumSize = 150;
  this->Frame2MinimumSize = 150;

  this->Frame1Visibility = 1;
  this->Frame2Visibility = 1;

  this->SeparatorSize = 4;
  this->SeparatorMargin = 2;
  this->SeparatorVisibility = 1;

  this->Size = 
    this->Frame1Size + this->Frame2Size + this->GetTotalSeparatorSize();

  this->Orientation = vtkKWSplitFrame::OrientationHorizontal;
  this->ExpandFrame = vtkKWSplitFrame::ExpandFrame2;
}

//----------------------------------------------------------------------------
vtkKWSplitFrame::~vtkKWSplitFrame()
{
  if (this->Frame1)
    {
    this->Frame1->Delete();
    this->Frame1 = NULL;
    }

  if (this->Separator)
    {
    this->Separator->Delete();
    this->Separator = NULL;
    }

  if (this->Frame2)
    {
    this->Frame2->Delete();
    this->Frame2 = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->SetConfigurationOptionAsInt("-width", 200);
  this->SetConfigurationOptionAsInt("-height", 100);

  this->Frame1->SetParent(this);
  this->Frame1->Create(app);

  this->Separator->SetParent(this);
  this->Separator->Create(app);
  this->Separator->SetBorderWidth(2);
  this->Separator->SetReliefToRaised();

  this->Frame2->SetParent(this);
  this->Frame2->Create(app);
  
  this->Pack();

  this->AddBindings();

  // Setup the cursor to indication an action associated with the separator. 
#ifdef _WIN32
  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    this->Separator->SetConfigurationOption("-cursor", "size_we");
    }
  else
    {
    this->Separator->SetConfigurationOption("-cursor", "size_ns");
    }
#else
  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {  
    this->Separator->SetConfigurationOption("-cursor", "sb_h_double_arrow");
    }
  else
    {
    this->Separator->SetConfigurationOption("-cursor", "sb_v_double_arrow");
    }
#endif

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::AddSeparatorBindings()
{
  if (this->Separator)
    {
    this->Separator->SetBinding("<B1-Motion>", this, "DragCallback");
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::RemoveSeparatorBindings()
{
  if (this->Separator)
    {
    this->Separator->RemoveBinding("<B1-Motion>");
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::AddBindings()
{
  this->AddSeparatorBindings();

  this->SetBinding("<Configure>", this, "ConfigureCallback");
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::RemoveBindings()
{
  this->RemoveSeparatorBindings();

  this->RemoveBinding("<Configure>");
}

//----------------------------------------------------------------------------
int vtkKWSplitFrame::GetInternalMarginHorizontal()
{
  return this->GetBorderWidth() + this->GetPadX();
}

//----------------------------------------------------------------------------
int vtkKWSplitFrame::GetInternalMarginVertical()
{
  return this->GetBorderWidth() + this->GetPadY();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::ReConfigure()
{
  if (this->IsCreated())
    {
    this->Size = 
      this->Frame1Size + this->Frame2Size + this->GetTotalSeparatorSize();
    if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
      {
      int margin = this->GetInternalMarginHorizontal();
      this->SetWidth(this->Size + margin * 2);
      }
    else
      {
      int margin = this->GetInternalMarginVertical();
      this->SetHeight(this->Size + margin * 2);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::ConfigureCallback()
{
  int tmp;
  int size;

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    size = atoi(this->Script( "winfo width %s", this->GetWidgetName()));
    size -= this->GetInternalMarginHorizontal() * 2;
    }
  else
    {
    size = atoi(this->Script( "winfo height %s", this->GetWidgetName()));
    size -= this->GetInternalMarginVertical() * 2;
    }

  // If size == 1 then the widget has not been packed, it will be later
  // and the <Configure> event will bring us back here with the correct size

  if (size <= 1)
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
    this->ReConfigure();
    this->Pack();
    return;
    }

  // Size of second frame
  // Adjust the first frame size if needed

  if (this->ExpandFrame == vtkKWSplitFrame::ExpandFrame2)
    {
    tmp = size - this->Frame1Size - this->GetTotalSeparatorSize();
    if (tmp < this->Frame2MinimumSize)
      {
      tmp = this->Frame2MinimumSize;
      this->Frame1Size = size - this->Frame2Size-this->GetTotalSeparatorSize();
      }
    this->Frame2Size = tmp;
    }
  else if (this->ExpandFrame == vtkKWSplitFrame::ExpandFrame1)
    {
    tmp = size - this->Frame2Size - this->GetTotalSeparatorSize();
    if (tmp < this->Frame1MinimumSize)
      {
      tmp = this->Frame1MinimumSize;
      this->Frame2Size = size - this->Frame1Size-this->GetTotalSeparatorSize();
      }
    this->Frame1Size = tmp;
    }
  else
    {
    tmp = size - this->Size;
    int frame1size = this->Frame1Size + tmp / 2;
    int frame2size = this->Frame2Size + (tmp - tmp / 2);
    if (frame1size < this->Frame1MinimumSize)
      {
      frame2size -= (this->Frame1MinimumSize - frame1size);
      frame1size = this->Frame1MinimumSize;
      }
    if (frame2size < this->Frame2MinimumSize)
      {
      frame1size -= (this->Frame2MinimumSize - frame2size);
      frame2size = this->Frame2MinimumSize;
      }
    this->Frame1Size = frame1size;
    this->Frame2Size = frame2size;
    }

  this->Size = size;

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::DragCallback()
{
  int size, s, smin, tmp;

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    size = atoi(this->Script( "winfo width %s", this->GetWidgetName()));
    size -= this->GetInternalMarginHorizontal() * 2;
    }
  else
    {
    size = atoi(this->Script( "winfo height %s", this->GetWidgetName()));
    size -= this->GetInternalMarginVertical() * 2;
    }

  this->Size = size;

  // Get the position of the widget in screen

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    smin = atoi(this->Script( "winfo rootx %s",this->GetWidgetName()));
    }
  else
    {
    smin = atoi(this->Script( "winfo rooty %s",this->GetWidgetName()));
    } 

  // Get the position of the mouse in screen

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
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

  int halfSepSize1 = this->GetTotalSeparatorSize() / 2;
  int halfSepSize2 = this->GetTotalSeparatorSize() - halfSepSize1;

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
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
    this->Frame1Size = s - halfSepSize1;
    this->Frame2Size = size - s - halfSepSize2;
    }
  else
    {
    tmp = this->Frame2MinimumSize + halfSepSize2;
    if (s < tmp)
      {
      s = tmp;
      }
    tmp = size - this->Frame1MinimumSize - halfSepSize1;
    if (s > tmp)
      {
      s = tmp;
      }
    this->Frame2Size = s - halfSepSize2;
    this->Frame1Size = size - s - halfSepSize1;
    }

  this->Pack();
}


//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame1MinimumSize(int minSize)
{
  if (this->Frame1MinimumSize == minSize)
    {
    return;
    }

  this->Frame1MinimumSize = minSize;

  if (this->Frame1Size < this->Frame1MinimumSize)
    {
    this->SetFrame1Size(this->Frame1MinimumSize);
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

  this->Frame2Size = 
    this->Size - this->Frame1Size - this->GetTotalSeparatorSize();

  if (this->Frame2Size < this->Frame2MinimumSize)
    {
    this->Frame2Size = this->Frame2MinimumSize;
    this->ReConfigure();
    }

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame2MinimumSize(int minSize)
{
  if (this->Frame2MinimumSize == minSize)
    {
    return;
    }

  this->Frame2MinimumSize = minSize;

  if (this->Frame2Size < this->Frame2MinimumSize)
    {
    this->SetFrame2Size(this->Frame2MinimumSize);
    }
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrame2Size(int size)
{
  if (this->Frame2Size == size)
    {
    return;
    }

  this->Frame2Size = size;

  this->Frame1Size = 
    this->Size - this->Frame2Size - this->GetTotalSeparatorSize();

  if (this->Frame1Size < this->Frame1MinimumSize)
    {
    this->Frame1Size = this->Frame1MinimumSize;
    this->ReConfigure();
    }

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetSeparatorSize(int size)
{
  if (this->SeparatorSize == size)
    {
    return;
    }

  this->SeparatorSize = size;
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
  this->SetFrame1Size(this->Frame1Size);
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetSeparatorVisibility(int flag)
{
  if (this->SeparatorVisibility == flag)
    {
    return;
    }

  this->SeparatorVisibility = flag;
  this->Modified();
  this->Pack();

  if (this->SeparatorVisibility)
    {
    this->AddSeparatorBindings();
    }
  else
    {
    this->RemoveSeparatorBindings();
    }
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
  this->Pack();
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
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  int total_separator_size, separator_size, separator_margin;
  int frame1_size, frame2_size;

  if (this->Frame1Visibility && this->Frame2Visibility)
    {
    if (this->SeparatorVisibility)
      {
      separator_size = this->SeparatorSize;
      separator_margin = this->SeparatorMargin;
      total_separator_size = this->GetTotalSeparatorSize();
      frame1_size = this->Frame1Size;
      frame2_size = this->Frame2Size;
      }
    else
      {
      separator_size = 0;
      separator_margin = 0;
      total_separator_size = this->SeparatorMargin;
      int remaining = this->GetTotalSeparatorSize() - total_separator_size;
      frame1_size = this->Frame1Size + (remaining / 2);
      frame2_size = this->Frame2Size + (remaining - remaining / 2);
      }
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

  int margin_h = 0; // this->GetInternalMarginHorizontal();
  int margin_v = 0; // this->GetInternalMarginVertical();

  if (this->Frame1Visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)  
      {
      this->Script("place %s -x %d -y %d -width %d -relheight 1.0 -height -%d",
                   this->Frame1->GetWidgetName(), 
                   margin_h, 
                   margin_v, 
                   frame1_size, margin_v * 2);
      }
    else
      {
      this->Script("place %s -x %d -y %d -height %d -relwidth 1.0 -width -%d",
                   this->Frame1->GetWidgetName(), 
                   margin_h,
                   margin_v + frame2_size + total_separator_size, 
                   frame1_size, margin_h * 2);
      }
    }
  else
    {
    this->Script("place forget %s", this->Frame1->GetWidgetName());
    }

  if (this->Frame1Visibility && 
      this->Frame2Visibility && 
      this->SeparatorVisibility)
    {
    if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)  
      {
      this->Script("place %s -x %d -y %d -width %d -relheight 1.0 -height -%d",
                   this->Separator->GetWidgetName(), 
                   margin_h + frame1_size + separator_margin,
                   margin_v,
                   separator_size, margin_v * 2);
      }
    else
      {
      this->Script("place %s -x %d -y %d -height %d -relwidth 1.0 -width -%d",
                   this->Separator->GetWidgetName(), 
                   margin_h,
                   margin_v + frame2_size + separator_margin,
                   separator_size, margin_h * 2);
      }
    }
  else
    {
    this->Script("place forget %s", this->Separator->GetWidgetName());
    }

  if (this->Frame2Visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)  
      {
      this->Script("place %s -x %d -y %d -width %d -relheight 1.0 -height -%d",
                   this->Frame2->GetWidgetName(), 
                   margin_h + frame1_size + total_separator_size, 
                   margin_v,
                   frame2_size, margin_v * 2);
      }
    else
      {
      this->Script("place %s -x %d -y %d -height %d -relwidth 1.0 -width -%d",
                   this->Frame2->GetWidgetName(), 
                   margin_h,
                   margin_v,
                   frame2_size, margin_h * 2);
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
  os << indent << "SeparatorVisibility: " << (this->SeparatorVisibility ? "On" : "Off") << endl;

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    os << indent << "Orientation: Horizontal\n";
    }
  else
    {
    os << indent << "Orientation: Vertical\n";
    }
  if (this->ExpandFrame == vtkKWSplitFrame::OrientationHorizontal)
    {
    os << indent << "ExpandFrame: Horizontal\n";
    }
  else
    {
    os << indent << "ExpandFrame: Vertical\n";
    }
}

