/*=========================================================================

  Module:    vtkKWSelectionFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSelectionFrame.h"
#include "vtkObjectFactory.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWIcon.h"
#include "vtkKWToolbarSet.h"

vtkStandardNewMacro(vtkKWSelectionFrame);
vtkCxxRevisionMacro(vtkKWSelectionFrame, "1.23");

//----------------------------------------------------------------------------
vtkKWSelectionFrame::vtkKWSelectionFrame()
{
  this->TitleBar              = vtkKWFrame::New();
  this->Title                 = vtkKWLabel::New();
  this->SelectionList         = vtkKWMenuButton::New();
  this->ToolbarSet            = vtkKWToolbarSet::New();
  this->TitleBarRightSubframe = vtkKWFrame::New();
  this->BodyFrame             = vtkKWFrame::New();

  this->SelectionListCommand  = NULL;
  this->SelectCommand         = NULL;

  this->TitleColor[0]                   = 1.0;
  this->TitleColor[1]                   = 1.0;
  this->TitleColor[2]                   = 1.0;

  this->TitleSelectedColor[0]           = 1.0;
  this->TitleSelectedColor[1]           = 1.0;
  this->TitleSelectedColor[2]           = 1.0;

  this->TitleBackgroundColor[0]         = 0.6;
  this->TitleBackgroundColor[1]         = 0.6;
  this->TitleBackgroundColor[2]         = 0.6;

  this->TitleBackgroundSelectedColor[0] = 0.0;
  this->TitleBackgroundSelectedColor[1] = 0.0;
  this->TitleBackgroundSelectedColor[2] = 0.5;

  this->Selected          = 0;
  this->ShowSelectionList = 1;
  this->ShowToolbarSet    = 0;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame::~vtkKWSelectionFrame()
{
  if (this->TitleBar)
    {
    this->TitleBar->Delete();
    this->TitleBar = NULL;
    }

  if (this->Title)
    {
    this->Title->Delete();
    this->Title = NULL;
    }

  if (this->SelectionList)
    {
    this->SelectionList->Delete();
    this->SelectionList = NULL;
    }

  if (this->TitleBarRightSubframe)
    {
    this->TitleBarRightSubframe->Delete();
    this->TitleBarRightSubframe = NULL;
    }

  if (this->ToolbarSet)
    {
    this->ToolbarSet->Delete();
    this->ToolbarSet = NULL;
    }

  if (this->BodyFrame)
    {
    this->BodyFrame->Delete();
    this->BodyFrame = NULL;
    }

  if (this->SelectionListCommand)
    {
    delete [] this->SelectionListCommand;
    this->SelectionListCommand = NULL;
    }

  if (this->SelectCommand)
    {
    delete [] this->SelectCommand;
    this->SelectCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Script("%s config -bd 3 -relief ridge",  this->GetWidgetName());

  // The title bar

  this->TitleBar->SetParent(this);
  this->TitleBar->Create(app, NULL);

  // The selection button

  this->SelectionList->SetParent(this->TitleBar);
  this->SelectionList->Create(app, NULL);
  this->SelectionList->IndicatorOff();
  this->SelectionList->SetImageOption(vtkKWIcon::ICON_EXPAND);

  // The title itself

  this->Title->SetParent(this->TitleBar);
  this->Title->Create(app, NULL);
  this->Title->SetLabel("<Click to Select>");
  
  // The subframe on the right

  this->TitleBarRightSubframe->SetParent(this->TitleBar);
  this->TitleBarRightSubframe->Create(app, NULL);

  // The toobar

  this->ToolbarSet->SetParent(this);
  this->ToolbarSet->ShowBottomSeparatorOff();
  this->ToolbarSet->Create(app, NULL);

  // The body frame

  this->BodyFrame->SetParent(this);
  this->BodyFrame->Create(app, "-bg black");

  // Pack

  this->Pack();

  // Update colors

  this->UpdateColors();
  
  // Bind

  this->Bind();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::Pack()
{
  if (!this->IsAlive())
    {
    return;
    }

  this->UnpackChildren();

  ostrstream tk_cmd;

  if (this->TitleBar->IsCreated())
    {
    tk_cmd << "pack " << this->TitleBar->GetWidgetName()
           << " -side top -fill x -expand no" << endl;
    }

  if (this->ShowSelectionList && this->SelectionList->IsCreated())
    {
    tk_cmd << "pack " << this->SelectionList->GetWidgetName()
           << " -side left -anchor w -fill y -padx 1 -pady 1" << endl;
    }

  if (this->Title->IsCreated())
    {
    tk_cmd << "pack " << this->Title->GetWidgetName()
           << " -side left -anchor w -fill y" << endl;
    }
  
  if (this->TitleBarRightSubframe->IsCreated())
    {
    tk_cmd << "pack " << this->TitleBarRightSubframe->GetWidgetName()
           << " -side right -anchor e -padx 4" << endl;
    }
  
  if (this->ShowToolbarSet && this->ToolbarSet->IsCreated())
    {
    tk_cmd << "pack " << this->ToolbarSet->GetWidgetName()
           << " -side top -fill x -expand no -padx 1 -pady 1" << endl;
    this->ToolbarSet->PackToolbars();
    }

  if (this->BodyFrame->IsCreated())
    {
    tk_cmd << "pack " << this->BodyFrame->GetWidgetName()
           << " -side top -fill both -expand yes" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::Bind()
{
  if (!this->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  if (this->TitleBar && this->TitleBar->IsCreated())
    {
    tk_cmd << "bind " << this->TitleBar->GetWidgetName() 
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " SelectCallback}" << endl;
    }

  if (this->SelectionList && this->SelectionList->IsCreated())
    {
    tk_cmd << "bind " << this->SelectionList->GetWidgetName() 
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " SelectCallback}" << endl;
    }

  if (this->ToolbarSet && this->ToolbarSet->IsCreated())
    {
    tk_cmd << "bind " << this->ToolbarSet->GetWidgetName() 
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " SelectCallback}" << endl;
    }

  if (this->Title && this->Title->IsCreated())
    {
    tk_cmd << "bind " << this->Title->GetWidgetName() 
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " SelectCallback}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UnBind()
{
  if (!this->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  if (this->TitleBar && this->TitleBar->IsCreated())
    {
    tk_cmd << "bind " << this->TitleBar->GetWidgetName() 
           << " <ButtonPress-1> {}" << endl;
    }

  if (this->SelectionList && this->SelectionList->IsCreated())
    {
    tk_cmd << "bind " << this->SelectionList->GetWidgetName() 
           << " <ButtonPress-1> {}" << endl;
    }

  if (this->ToolbarSet && this->ToolbarSet->IsCreated())
    {
    tk_cmd << "bind " << this->ToolbarSet->GetWidgetName() 
           << " <ButtonPress-1> {}" << endl;
    }

  if (this->Title && this->Title->IsCreated())
    {
    tk_cmd << "bind " << this->Title->GetWidgetName() 
           << " <ButtonPress-1> {}" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitle(const char *title)
{
  this->Title->SetLabel(title);
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrame::GetTitle()
{
  return this->Title->GetLabel();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrame::SetColor(
  float *color, float r, float g, float b)
{
  if ((r == color[0] && g == color[1] &&  b == color[2]) ||
      (r < 0.0 || r > 1.0) || (g < 0.0 || g > 1.0) || (b < 0.0 || b > 1.0))
    {
    return 0;
    }

  color[0] = r;
  color[1] = g;
  color[2] = b;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleSelectedColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleBackgroundColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundSelectedColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleBackgroundSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelected(int arg)
{
  if (this->Selected == arg)
    {
    return;
    }

  this->Selected = arg;

  this->Modified();
  this->UpdateColors();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetShowSelectionList(int arg)
{
  if (this->ShowSelectionList == arg)
    {
    return;
    }

  this->ShowSelectionList = arg;

  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetShowToolbarSet(int arg)
{
  if (this->ShowToolbarSet == arg)
    {
    return;
    }

  this->ShowToolbarSet = arg;

  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateColors()
{
  if (!this->IsCreated())
    {
    return;
    }

  float *fgcolor, *bgcolor;

  if (this->Selected)
    {
    fgcolor = this->TitleSelectedColor;
    bgcolor = this->TitleBackgroundSelectedColor;
    }
  else
    {
    fgcolor = this->TitleColor;
    bgcolor = this->TitleBackgroundColor;
    }

  this->TitleBar->SetBackgroundColor(
    bgcolor[0], bgcolor[1], bgcolor[2]);

  this->Title->SetBackgroundColor(
    bgcolor[0], bgcolor[1], bgcolor[2]);

  this->Title->SetForegroundColor(
    fgcolor[0], fgcolor[1], fgcolor[2]);

  this->TitleBarRightSubframe->SetBackgroundColor(
    bgcolor[0], bgcolor[1], bgcolor[2]);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectionList(int num, const char **list)
{
  if (!this->SelectionList->IsCreated())
    {
    vtkErrorMacro(
      "Selection frame must be created before selection list can be set");
    return;
    }
  
  this->SelectionList->GetMenu()->DeleteAllMenuItems();
  
  int i;
  for (i = 0; i < num; i++)
    {
    ostrstream cbk;
    cbk << "SelectionListCallback {" << list[i] << "}" << ends;
    this->SelectionList->AddCommand(list[i], this, cbk.str());
    cbk.rdbuf()->freeze(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectionListCommand(vtkKWObject *object,
                                                  const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionListCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectCommand(vtkKWObject *object,
                                                    const char *method)
{
  this->SetObjectMethodCommand(&this->SelectCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SelectionListCallback(const char *menuItem)
{
  if (this->SelectionListCommand)
    {
    this->Script("eval {%s {%s} %s}",
                 this->SelectionListCommand, menuItem, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SelectCallback()
{
  if (this->GetSelected())
    {
    return;
    }

  this->SelectedOn();

  if (this->SelectCommand)
    {
    this->Script("eval {%s %s}",
                 this->SelectCommand, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->TitleBar);
  this->PropagateEnableState(this->SelectionList);
  this->PropagateEnableState(this->Title);
  this->PropagateEnableState(this->TitleBarRightSubframe);
  this->PropagateEnableState(this->ToolbarSet);
  this->PropagateEnableState(this->BodyFrame);

  if (this->Enabled)
    {
    this->Bind();
    }
  else
    {
    this->UnBind();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "BodyFrame: " << this->BodyFrame << endl;
  os << indent << "TitleBarRightSubframe: " << this->TitleBarRightSubframe
     << endl;
  os << indent << "SelectionList: " << this->SelectionList << endl;
  os << indent << "ToolbarSet: " << this->ToolbarSet << endl;
  os << indent << "TitleColor: ("
     << this->TitleColor[0] << ", " 
     << this->TitleColor[1] << ", " 
     << this->TitleColor[2] << ")" << endl;
  os << indent << "TitleSelectedColor: ("
     << this->TitleSelectedColor[0] << ", " 
     << this->TitleSelectedColor[1] << ", " 
     << this->TitleSelectedColor[2] << ")" << endl;
  os << indent << "TitleBackgroundColor: ("
     << this->TitleBackgroundColor[0] << ", " 
     << this->TitleBackgroundColor[1] << ", " 
     << this->TitleBackgroundColor[2] << ")" << endl;
  os << indent << "TitleBackgroundSelectedColor: ("
     << this->TitleBackgroundSelectedColor[0] << ", " 
     << this->TitleBackgroundSelectedColor[1] << ", " 
     << this->TitleBackgroundSelectedColor[2] << ")" << endl;
  os << indent << "Selected: " << (this->Selected ? "On" : "Off") << endl;
  os << indent << "ShowSelectionList: " << (this->ShowSelectionList ? "On" : "Off") << endl;
  os << indent << "ShowToolbarSet: " << (this->ShowToolbarSet ? "On" : "Off") << endl;
}

