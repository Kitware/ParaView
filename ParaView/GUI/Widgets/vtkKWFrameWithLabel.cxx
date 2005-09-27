/*=========================================================================

  Module:    vtkKWFrameWithLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWFrameWithLabel.h"

#include "vtkKWApplication.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrameWithLabel );
vtkCxxRevisionMacro(vtkKWFrameWithLabel, "1.3");

int vtkKWFrameWithLabel::DefaultLabelCase = vtkKWFrameWithLabel::LabelCaseUppercaseFirst;
int vtkKWFrameWithLabel::DefaultLabelFontWeight = vtkKWFrameWithLabel::LabelFontWeightBold;
int vtkKWFrameWithLabel::DefaultAllowFrameToCollapse = 1;

//----------------------------------------------------------------------------
vtkKWFrameWithLabel::vtkKWFrameWithLabel()
{
  this->Border     = vtkKWFrame::New();
  this->Groove     = vtkKWFrame::New();
  this->Border2    = vtkKWFrame::New();
  this->Frame      = vtkKWFrame::New();
  this->LabelFrame = vtkKWFrame::New();
  this->Label      = vtkKWLabelWithLabel::New();
  this->Icon       = vtkKWLabel::New();
  this->IconData   = vtkKWIcon::New();

  this->AllowFrameToCollapse = 1;
  this->LimitedEditionModeIconVisibility = 0;
}

//----------------------------------------------------------------------------
vtkKWFrameWithLabel::~vtkKWFrameWithLabel()
{
  this->Icon->Delete();
  this->IconData->Delete();
  this->Label->Delete();
  this->Frame->Delete();
  this->LabelFrame->Delete();
  this->Border->Delete();
  this->Border2->Delete();
  this->Groove->Delete();
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWFrameWithLabel::GetLabel()
{
  if (this->Label)
    {
    return this->Label->GetWidget();
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWFrameWithLabel::GetLabelIcon()
{
  if (this->Label)
    {
    return this->Label->GetLabel();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::SetLabelText(const char *text)
{
  if (!text)
    {
    return;
    }

  if (vtkKWFrameWithLabel::DefaultLabelCase == 
      vtkKWFrameWithLabel::LabelCaseUserSpecified)
    {
    this->GetLabel()->SetText(text);
    }
  else
    {
    vtksys_stl::string res;
    switch (vtkKWFrameWithLabel::DefaultLabelCase)
      {
      case vtkKWFrameWithLabel::LabelCaseUppercaseFirst:
        res = vtksys::SystemTools::CapitalizedWords(text);
        break;
      case vtkKWFrameWithLabel::LabelCaseLowercaseFirst:
        res = vtksys::SystemTools::UnCapitalizedWords(text);
        break;
      }
    this->GetLabel()->SetText(res.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::AdjustMarginCallback()
{
  if (this->IsCreated())
    {
    // Get the height of the label frame, and share it between
    // the two borders (frame).

    int height = atoi(this->Script("winfo reqheight %s", 
                                   this->LabelFrame->GetWidgetName()));

    // If the frame has not been packed yet, reqheight will return 1,
    // so try the hard way by checking what's inside the pack, provided
    // that it's simple (i.e. packed in a single row or column)

    if (height <= 1) 
      {
      int width;
      vtkKWTkUtilities::GetSlavesBoundingBoxInPack(
        this->LabelFrame, &width, &height);
      }

    // Don't forget the show/hide collapse icon, it might be bigger than
    // the LabelFrame contents (really ?)

    if (vtkKWFrameWithLabel::DefaultAllowFrameToCollapse && 
        this->AllowFrameToCollapse &&
        height < this->IconData->GetHeight())
      {
      height = this->IconData->GetHeight();
      }

    int border_h = height / 2;
    int border2_h = height / 2;
#ifdef _WIN32
    border_h++;
#else
    border2_h++;
#endif

    this->Border->SetHeight(border_h);
    this->Border2->SetHeight(border2_h);

    if (vtkKWFrameWithLabel::DefaultAllowFrameToCollapse && 
        this->AllowFrameToCollapse)
      {
      this->Script("place %s -relx 1 -x %d -rely 0 -y %d -anchor center",
                   this->Icon->GetWidgetName(),
                   -this->IconData->GetWidth() -1,
                   border_h + 1);    
      this->Script("raise %s", this->Icon->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->Border->SetParent(this);
  this->Border->Create(app);

  this->Groove->SetParent(this);
  this->Groove->Create(app);
  this->Groove->SetReliefToGroove();
  this->Groove->SetBorderWidth(2);

  this->Border2->SetParent(this->Groove);
  this->Border2->Create(app);

  this->Frame->SetParent(this->Groove);
  this->Frame->Create(app);

  this->LabelFrame->SetParent(this);
  this->LabelFrame->Create(app);

  this->Label->SetParent(this->LabelFrame);
  this->Label->Create(app);
  this->Label->SetBorderWidth(0);
  this->Label->ExpandWidgetOff();

  vtkKWLabel *label = this->GetLabel();
  label->SetBorderWidth(1);
  label->SetPadX(0);
  label->SetPadY(0);

  // At this point, although this->Label (a labeled label) has been created,
  // UpdateEnableState() has been called already and ShowLabelOff() has been
  // called on the label. Therefore, the label of this->Label was not created
  // since it is lazy created/allocated on the fly only when needed.
  // Force label icon to be created now, so that we can set its image option.

  this->Label->LabelVisibilityOn();

  label = this->GetLabelIcon();
  label->SetImageToPredefinedIcon(vtkKWIcon::IconLock);
  label->SetBorderWidth(0);
  label->SetPadX(0);
  label->SetPadY(0);

  const char *lem_name = app->GetLimitedEditionModeName() 
    ? app->GetLimitedEditionModeName() : "Limited Edition";
  
  ostrstream balloon_str;
  balloon_str << "This feature is not available in \"" << lem_name 
              << "\" mode." << ends;
  this->GetLabelIcon()->SetBalloonHelpString(balloon_str.str());
  balloon_str.rdbuf()->freeze(0);

  if (vtkKWFrameWithLabel::DefaultLabelFontWeight ==
      vtkKWFrameWithLabel::LabelFontWeightBold)
    {
    vtkKWTkUtilities::ChangeFontWeightToBold(this->GetLabel());
    }

  this->IconData->SetImage(vtkKWIcon::IconShrink);

  this->Icon->SetParent(this);
  this->Icon->Create(app);
  this->Icon->SetImageToIcon(this->IconData);
  this->Icon->SetBalloonHelpString("Shrink or expand the frame");
  
  this->Script("pack %s -fill x -side top", this->Border->GetWidgetName());
  this->Script("pack %s -fill x -side top", this->Groove->GetWidgetName());
  this->Script("pack %s -fill x -side top", this->Border2->GetWidgetName());
  this->Script("pack %s -padx 2 -pady 2 -fill both -expand yes",
               this->Frame->GetWidgetName());
  this->Script(
    "pack %s -anchor nw -side left -fill both -expand y -padx 2 -pady 0",
    this->Label->GetWidgetName());
  this->Script("place %s -relx 0 -x 5 -y 0 -anchor nw",
               this->LabelFrame->GetWidgetName());

  this->Script("raise %s", this->Label->GetWidgetName());

  if (vtkKWFrameWithLabel::DefaultAllowFrameToCollapse && 
      this->AllowFrameToCollapse)
    {
    this->Icon->SetBinding("<ButtonRelease-1>",this,"CollapseButtonCallback");
    }

  // If the label frame get resize, reset the margins

  vtksys_stl::string callback("catch {");
  callback += this->GetTclName();
  callback += " AdjustMarginCallback}";
  this->LabelFrame->SetBinding("<Configure>", NULL, callback.c_str());

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::ExpandFrame()
{
  if (this->Frame && this->Frame->IsCreated())
    {
    this->Script("pack %s -fill both -expand yes",
                 this->Frame->GetWidgetName());
    }
  if (this->IconData && this->Icon)
    {
    this->IconData->SetImage(vtkKWIcon::IconShrink);
    this->Icon->SetImageToIcon(this->IconData);
    }
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::CollapseFrame()
{
  if (this->Frame && this->Frame->IsCreated())
    {
    this->Script("pack forget %s", 
                 this->Frame->GetWidgetName());
    }
  if (this->IconData && this->Icon)
    {
    this->IconData->SetImage(vtkKWIcon::IconExpand);
    this->Icon->SetImageToIcon(this->IconData);
    }
}

//----------------------------------------------------------------------------
int vtkKWFrameWithLabel::IsFrameCollapsed()
{
  return (this->Frame && this->Frame->IsCreated() && !this->Frame->IsPacked());
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::CollapseButtonCallback()
{
  if (this->IsFrameCollapsed())
    {
    this->ExpandFrame();
    }
  else
    {
    this->CollapseFrame();
    }
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::SetDefaultAllowFrameToCollapse(int arg) 
{ 
  vtkKWFrameWithLabel::DefaultAllowFrameToCollapse = arg; 
}

//----------------------------------------------------------------------------
int vtkKWFrameWithLabel::GetDefaultAllowFrameToCollapse() 
{ 
  return vtkKWFrameWithLabel::DefaultAllowFrameToCollapse; 
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::SetDefaultLabelFontWeight(int arg) 
{ 
  vtkKWFrameWithLabel::DefaultLabelFontWeight = arg; 
}

//----------------------------------------------------------------------------
int vtkKWFrameWithLabel::GetDefaultLabelFontWeight() 
{ 
  return vtkKWFrameWithLabel::DefaultLabelFontWeight; 
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::SetDefaultLabelCase(int v) 
{ 
  vtkKWFrameWithLabel::DefaultLabelCase = v;
}

//----------------------------------------------------------------------------
int vtkKWFrameWithLabel::GetDefaultLabelCase() 
{ 
  return vtkKWFrameWithLabel::DefaultLabelCase;
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::SetLimitedEditionModeIconVisibility(int arg)
{
  if (this->LimitedEditionModeIconVisibility == arg)
    {
    return;
    }

  this->LimitedEditionModeIconVisibility = arg;
  this->Modified();

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Disable only the label part of the labeled label
  // (we want the icon not to look disabled)

  this->PropagateEnableState(this->GetLabel());

  int limited = (this->GetApplication() && 
                 this->GetApplication()->GetLimitedEditionMode());
  
  if (limited && this->LimitedEditionModeIconVisibility && !this->GetEnabled())
    {
    this->Label->LabelVisibilityOn();
    }
  else
    {
    this->Label->LabelVisibilityOff();
    }
  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->LabelFrame);
  this->PropagateEnableState(this->Border);
  this->PropagateEnableState(this->Border2);
  this->PropagateEnableState(this->Groove);
  this->PropagateEnableState(this->Icon);
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet* vtkKWFrameWithLabel::GetDragAndDropTargetSet()
{
  int exist = this->HasDragAndDropTargetSet();
  vtkKWDragAndDropTargetSet *targets = this->Superclass::GetDragAndDropTargetSet();
  if (!exist)
    {
    targets->SetSourceAnchor(this->GetLabel());
    }
  return targets;
}

//----------------------------------------------------------------------------
void vtkKWFrameWithLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AllowFrameToCollapse: " 
     << (this->AllowFrameToCollapse ? "On" : "Off") << endl;
  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "LabelFrame: " << this->LabelFrame << endl;
  os << indent << "Label: " << this->Label << endl;
  os << indent << "LimitedEditionModeIconVisibility: " 
     << (this->LimitedEditionModeIconVisibility ? "On" : "Off") << endl;
}
