/*=========================================================================

  Module:    vtkKWLabeledFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWLabeledFrame.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabeledFrame );
vtkCxxRevisionMacro(vtkKWLabeledFrame, "1.38.2.1");

int vtkKWLabeledFrameCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledFrame::vtkKWLabeledFrame()
{
  this->CommandFunction = vtkKWLabeledFrameCommand;

  this->Border     = vtkKWWidget::New();
  this->Groove     = vtkKWWidget::New();
  this->Border2    = vtkKWWidget::New();
  this->Frame      = vtkKWFrame::New();
  this->LabelFrame = vtkKWFrame::New();
  this->Label      = vtkKWLabeledLabel::New();
  this->Icon       = vtkKWLabel::New();
  this->IconData   = vtkKWIcon::New();

  this->Displayed     = 1;
  this->ShowHideFrame = 0;
  this->ShowIconInLimitedEditionMode = 0;

  this->DragAndDropAnchor = this->GetLabel();
}

//----------------------------------------------------------------------------
vtkKWLabeledFrame::~vtkKWLabeledFrame()
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
vtkKWLabel* vtkKWLabeledFrame::GetLabel()
{
  if (this->Label)
    {
    return this->Label->GetLabel2();
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWLabeledFrame::GetLabelIcon()
{
  if (this->Label)
    {
    return this->Label->GetLabel();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWLabeledFrame::SetLabel(const char *text)
{
  if (!text)
    {
    return;
    }

  const char *ptr;
  char *buffer = 0;

  if (vtkKWLabeledFrame::LabelCase == VTK_KW_LABEL_CASE_USER_SPECIFIED)
    {
    ptr = text;
    }
  else
    {
    buffer = vtkString::Duplicate(text);
    switch (vtkKWLabeledFrame::LabelCase)
      {
      case VTK_KW_LABEL_CASE_UPPERCASE_FIRST:
        vtkString::ToUpperFirst(buffer);
        break;
      case VTK_KW_LABEL_CASE_LOWERCASE_FIRST:
        vtkString::ToLowerFirst(buffer);
        break;
      }
    ptr = buffer;
    }

  this->GetLabel()->SetLabel(ptr);

  if (vtkKWLabeledFrame::LabelCase != VTK_KW_LABEL_CASE_USER_SPECIFIED)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledFrame::AdjustMargin()
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
      vtkKWTkUtilities::GetPackSlavesBbox(
        this->GetApplication()->GetMainInterp(),
        this->LabelFrame->GetWidgetName(),
        &width, &height);
      }

    // Don't forget the show/hide collapse icon, it might be bigger than
    // the LabelFrame contents (really ?)

    if (vtkKWLabeledFrame::AllowShowHide && this->ShowHideFrame &&
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

    this->Script("%s configure -height %d", 
                 this->Border->GetWidgetName(), border_h);
    this->Script("%s configure -height %d", 
                 this->Border2->GetWidgetName(), border2_h);

    if ( vtkKWLabeledFrame::AllowShowHide && this->ShowHideFrame )
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
void vtkKWLabeledFrame::Create(vtkKWApplication *app, const char* args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  this->Border->SetParent(this);
  this->Border->Create(app, "frame", "-bd 0 -relief flat");

  this->Groove->SetParent(this);
  this->Groove->Create(app, "frame", "-bd 2 -relief groove");

  this->Border2->SetParent(this->Groove);
  this->Border2->Create(app, "frame", "-bd 0 -relief flat");

  this->Frame->SetParent(this->Groove);
  this->Frame->Create(app, "");

  this->LabelFrame->SetParent(this);
  this->LabelFrame->Create(app, "");

  this->Label->SetParent(this->LabelFrame);
  this->Label->Create(app, "-bd 0");

  this->Script("%s config -bd 1 -pady 0 -padx 0", 
               this->GetLabel()->GetWidgetName());

  // At this point, although this->Label (a labeled label) has been created,
  // UpdateEnableState() has been called already and ShowLabelOff() has been
  // called on the label. Therefore, the label of this->Label was not created
  // since it is lazy created/allocated on the fly only when needed.
  // Force label icon to be created now, so that we can set its image option.

  this->Label->ShowLabelOn();
  this->GetLabelIcon()->SetImageOption(vtkKWIcon::ICON_LOCK);
  this->Script("%s config -bd 0 -pady 0 -padx 0", 
               this->GetLabelIcon()->GetWidgetName());

  const char *lem_name = app->GetLimitedEditionModeName() 
    ? app->GetLimitedEditionModeName() : "Limited Edition";
  
  ostrstream balloon_str;
  balloon_str << "This feature is not available in \"" << lem_name 
              << "\" mode." << ends;
  this->GetLabelIcon()->SetBalloonHelpString(balloon_str.str());
  balloon_str.rdbuf()->freeze(0);

  if (vtkKWLabeledFrame::BoldLabel)
    {
    vtkKWTkUtilities::ChangeFontWeightToBold(
      app->GetMainInterp(),
      this->GetLabel()->GetWidgetName());
    }

  this->IconData->SetImage(vtkKWIcon::ICON_SHRINK);

  this->Icon->SetParent(this);
  this->Icon->Create(app, "");
  this->Icon->SetImageOption(this->IconData);
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

  if ( vtkKWLabeledFrame::AllowShowHide && this->ShowHideFrame )
    {
    this->Script("bind %s <ButtonRelease-1> { %s PerformShowHideFrame }",
                 this->Icon->GetWidgetName(),
                 this->GetTclName());
    }

  // If the label frame get resize, reconfigure the margins

  this->Script("bind %s <Configure> { catch {%s AdjustMargin} }",
               this->LabelFrame->GetWidgetName(), this->GetTclName());

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLabeledFrame::PerformShowHideFrame()
{
  if ( this->Displayed )
    {
    this->Script("pack forget %s", this->Frame->GetWidgetName());
    this->Displayed = 0;
    this->IconData->SetImage(vtkKWIcon::ICON_EXPAND);
    }
  else
    {
    this->Script("pack %s -fill both -expand yes",
                 this->Frame->GetWidgetName());
    this->Displayed = 1;
    this->IconData->SetImage(vtkKWIcon::ICON_SHRINK);
   }
  this->Icon->SetImageOption(this->IconData);
}

//----------------------------------------------------------------------------
int vtkKWLabeledFrame::AllowShowHide = 0;

void vtkKWLabeledFrame::AllowShowHideOn() 
{ 
  vtkKWLabeledFrame::AllowShowHide = 1; 
}
void vtkKWLabeledFrame::AllowShowHideOff() 
{ 
  vtkKWLabeledFrame::AllowShowHide = 0; 
}

//----------------------------------------------------------------------------
int vtkKWLabeledFrame::BoldLabel = 0;

void vtkKWLabeledFrame::BoldLabelOn() 
{ 
  vtkKWLabeledFrame::BoldLabel = 1; 
}
void vtkKWLabeledFrame::BoldLabelOff() 
{ 
  vtkKWLabeledFrame::BoldLabel = 0; 
}

//----------------------------------------------------------------------------
int vtkKWLabeledFrame::LabelCase = 0;

void vtkKWLabeledFrame::SetLabelCase(int v) 
{ 
  vtkKWLabeledFrame::LabelCase = v;
}

int vtkKWLabeledFrame::GetLabelCase() 
{ 
  return vtkKWLabeledFrame::LabelCase;
}

//----------------------------------------------------------------------------
void vtkKWLabeledFrame::SetShowIconInLimitedEditionMode(int arg)
{
  if (this->ShowIconInLimitedEditionMode == arg)
    {
    return;
    }

  this->ShowIconInLimitedEditionMode = arg;
  this->Modified();

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLabeledFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Disable only the label part of the labeled label
  // (we want the icon not to look disabled)

  this->PropagateEnableState(this->GetLabel());

  int limited = (this->GetApplication() && 
                 this->GetApplication()->GetLimitedEditionMode());
  
  if (limited && this->ShowIconInLimitedEditionMode && !this->Enabled)
    {
    this->Label->ShowLabelOn();
    }
  else
    {
    this->Label->ShowLabelOff();
    }
  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->LabelFrame);
  this->PropagateEnableState(this->Border);
  this->PropagateEnableState(this->Border2);
  this->PropagateEnableState(this->Groove);
  this->PropagateEnableState(this->Icon);
}

//----------------------------------------------------------------------------
void vtkKWLabeledFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ShowHideFrame: " 
     << (this->ShowHideFrame ? "On" : "Off") << endl;
  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "LabelFrame: " << this->LabelFrame << endl;
  os << indent << "Label: " << this->Label << endl;
  os << indent << "ShowIconInLimitedEditionMode: " 
     << (this->ShowIconInLimitedEditionMode ? "On" : "Off") << endl;
}
