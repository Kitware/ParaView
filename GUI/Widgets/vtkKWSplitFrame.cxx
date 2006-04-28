/*=========================================================================

  Module:    vtkKWSplitFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSplitFrame.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

vtkStandardNewMacro( vtkKWSplitFrame );
vtkCxxRevisionMacro(vtkKWSplitFrame, "1.42");

//----------------------------------------------------------------------------
vtkKWSplitFrame::vtkKWSplitFrame()
{
  this->Frame1 = vtkKWFrame::New();
  this->Separator = vtkKWFrame::New();
  this->Frame2 = vtkKWFrame::New();

  this->Frame1Size = 250;
  this->Frame2Size = 250;

  this->Frame1MinimumSize = 50;
  this->Frame2MinimumSize = 50;

  this->Frame1Visibility = 1;
  this->Frame2Visibility = 1;

  this->SeparatorSize = 4;
  this->SeparatorMargin = 2;
  this->SeparatorVisibility = 1;

  this->Size = 
    this->Frame1Size + this->Frame2Size + this->GetTotalSeparatorSize();

  this->Orientation = vtkKWSplitFrame::OrientationHorizontal;
  this->ExpandableFrame = vtkKWSplitFrame::ExpandableFrame2;
  this->FrameLayout = vtkKWSplitFrame::FrameLayoutDefault;
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
void vtkKWSplitFrame::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SetConfigurationOptionAsInt("-width", 200);
  this->SetConfigurationOptionAsInt("-height", 100);

  this->Frame1->SetParent(this);
  this->Frame1->Create();

  this->Separator->SetParent(this);
  this->Separator->Create();
  this->Separator->SetBorderWidth(2);
  this->Separator->SetReliefToRaised();

  this->Frame2->SetParent(this);
  this->Frame2->Create();
  
  this->Pack();

  this->AddBindings();

  this->ConfigureSeparatorCursor();
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
void vtkKWSplitFrame::SetOrientation(int val)
{
  if (val < vtkKWSplitFrame::OrientationHorizontal)
    {
    val = vtkKWSplitFrame::OrientationHorizontal;
    }
  if (val > vtkKWSplitFrame::OrientationVertical)
    {
    val = vtkKWSplitFrame::OrientationVertical;
    }

  if (this->Orientation == val)
    {
    return;
    }

  this->Orientation = val;
  this->Modified();

  this->ConfigureSeparatorCursor();

  // If we are created already, make sure we forget all layout settings.
  // This can't be done each time we Pack(), otherwise nasty flickers occur.

  if (this->IsCreated())
    {
    this->Script("place forget %s", this->Frame1->GetWidgetName());
    this->Script("place forget %s", this->Separator->GetWidgetName());
    this->Script("place forget %s", this->Frame2->GetWidgetName());
    }

  this->ConfigureCallback();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::SetFrameLayout(int val)
{
  if (val < vtkKWSplitFrame::FrameLayoutDefault)
    {
    val = vtkKWSplitFrame::FrameLayoutDefault;
    }
  if (val > vtkKWSplitFrame::FrameLayoutSwapped)
    {
    val = vtkKWSplitFrame::FrameLayoutSwapped;
    }

  if (this->FrameLayout == val)
    {
    return;
    }

  this->FrameLayout = val;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSplitFrame::ConfigureSeparatorCursor()
{
  if (!this->Separator || !this->Separator->IsCreated())
    {
    return;
    }

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
  if (!this->IsCreated())
    {
    return;
    }

  int tmp;
  int size;

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    vtkKWTkUtilities::GetWidgetSize(this, &size, NULL);
    size -= this->GetInternalMarginHorizontal() * 2;
    }
  else
    {
    vtkKWTkUtilities::GetWidgetSize(this, NULL, &size);
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

  if (this->ExpandableFrame == vtkKWSplitFrame::ExpandableFrame2)
    {
    tmp = size - this->Frame1Size - this->GetTotalSeparatorSize();
    if (tmp < this->Frame2MinimumSize)
      {
      tmp = this->Frame2MinimumSize;
      this->Frame1Size = size - this->Frame2Size-this->GetTotalSeparatorSize();
      }
    this->Frame2Size = tmp;
    }
  else if (this->ExpandableFrame == vtkKWSplitFrame::ExpandableFrame1)
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
    vtkKWTkUtilities::GetWidgetSize(this, &size, NULL);
    size -= this->GetInternalMarginHorizontal() * 2;
    }
  else
    {
    vtkKWTkUtilities::GetWidgetSize(this, NULL, &size);
    size -= this->GetInternalMarginVertical() * 2;
    }

  this->Size = size;

  // Get the position of the widget in screen

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    vtkKWTkUtilities::GetWidgetCoordinates(this, &smin, NULL);
    }
  else
    {
    vtkKWTkUtilities::GetWidgetCoordinates(this, NULL, &smin);
    } 

  // Get the position of the mouse in screen

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    vtkKWTkUtilities::GetMousePointerCoordinates(this, &s, NULL);
    }
  else
    {
    vtkKWTkUtilities::GetMousePointerCoordinates(this, NULL, &s);
    }

  // Relative position.

  s -= smin;

  // Constraints on the window.

  int half_sep_size1 = this->GetTotalSeparatorSize() / 2;
  int half_sep_size2 = this->GetTotalSeparatorSize() - half_sep_size1;

  int frame1_new_size, frame2_new_size;
  int frame1_min_size, frame2_min_size;

  if (this->FrameLayout == vtkKWSplitFrame::FrameLayoutDefault)
    {
    frame1_min_size = this->Frame1MinimumSize;
    frame2_min_size = this->Frame2MinimumSize;
    }
  else
    {
    frame1_min_size = this->Frame2MinimumSize;
    frame2_min_size = this->Frame1MinimumSize;
    }

  if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)
    {
    tmp = frame1_min_size + half_sep_size1;
    if (s < tmp)
      {
      s = tmp;
      }
    tmp = size - frame2_min_size - half_sep_size2;
    if (s > tmp)
      {
      s = tmp;
      }
    frame1_new_size = s - half_sep_size1;
    frame2_new_size = size - s - half_sep_size2;
    }
  else
    {
    tmp = frame2_min_size + half_sep_size2;
    if (s < tmp)
      {
      s = tmp;
      }
    tmp = size - frame1_min_size - half_sep_size1;
    if (s > tmp)
      {
      s = tmp;
      }
    frame2_new_size = s - half_sep_size2;
    frame1_new_size = size - s - half_sep_size1;
    }

  if (this->FrameLayout == vtkKWSplitFrame::FrameLayoutDefault)
    {
    this->Frame1Size = frame1_new_size;
    this->Frame2Size = frame2_new_size;
    }
  else
    {
    this->Frame1Size = frame2_new_size;
    this->Frame2Size = frame1_new_size;
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
void vtkKWSplitFrame::SetSeparatorPosition(double pos)
{
  if (pos < 0.0)
    {
    pos = 0.0;
    }
  if (pos > 1.0)
    {
    pos = 1.0;
    }
  if (this->GetSeparatorPosition() == pos)
    {
    return;
    }

  double total_size = (double)(this->GetFrame1Size() + this->GetFrame2Size());
  this->SetFrame1Size((int)(pos * total_size));
}

//----------------------------------------------------------------------------
double vtkKWSplitFrame::GetSeparatorPosition()
{
  int total_size = this->GetFrame1Size() + this->GetFrame2Size();
  if (total_size)
    {
    return (double)this->GetFrame1Size() / (double)total_size;
    }
  return 0.0;
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

  int frame1_visibility = this->Frame1Visibility;
  int frame2_visibility = this->Frame2Visibility;

  vtkKWFrame *frame1 = this->Frame1;
  vtkKWFrame *frame2 = this->Frame2;

  // Compute the real size of each element, given the visibility parameters

  if (frame1_visibility && frame2_visibility)
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
    if (frame1_visibility)
      {
      frame1_size = this->Size;
      frame2_size = 0;
      }
    else if (frame2_visibility)
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

  // If we are swapped, switch Frame1 and Frame2

  if (this->FrameLayout == vtkKWSplitFrame::FrameLayoutSwapped)
    {
    int temp;

    temp = frame1_size;
    frame1_size = frame2_size;
    frame2_size = temp;

    temp = frame1_visibility;
    frame1_visibility = frame2_visibility;
    frame2_visibility = temp;
    
    frame1 = this->Frame2;
    frame2 = this->Frame1;
    }

  // Frame 1

  if (frame1_visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)  
      {
      this->Script("place %s -x %d -y %d -width %d -relheight 1.0 -height -%d",
                   frame1->GetWidgetName(), 
                   margin_h, 
                   margin_v, 
                   frame1_size, margin_v * 2);
      }
    else
      {
      this->Script("place %s -x %d -y %d -height %d -relwidth 1.0 -width -%d",
                   frame1->GetWidgetName(), 
                   margin_h,
                   margin_v + frame2_size + total_separator_size, 
                   frame1_size, margin_h * 2);
      }
    }
  else
    {
    this->Script("place forget %s", frame1->GetWidgetName());
    }

  // Separator

  if (frame1_visibility && 
      frame2_visibility && 
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

  // Frame 2

  if (frame2_visibility)
    {
    if (this->Orientation == vtkKWSplitFrame::OrientationHorizontal)  
      {
      this->Script("place %s -x %d -y %d -width %d -relheight 1.0 -height -%d",
                   frame2->GetWidgetName(), 
                   margin_h + frame1_size + total_separator_size, 
                   margin_v,
                   frame2_size, margin_v * 2);
      }
    else
      {
      this->Script("place %s -x %d -y %d -height %d -relwidth 1.0 -width -%d",
                   frame2->GetWidgetName(), 
                   margin_h,
                   margin_v,
                   frame2_size, margin_h * 2);
      }
    }
  else
    {
    this->Script("place forget %s", frame2->GetWidgetName());
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
  os << indent << "ExpandableFrame: " << this->GetExpandableFrame() << endl;
}

