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
#include "vtkKWPushButton.h"
#include "vtkKWIcon.h"
#include "vtkKWToolbarSet.h"

#include <vtksys/stl/list>
#include <vtksys/stl/string>

vtkStandardNewMacro(vtkKWSelectionFrame);
vtkCxxRevisionMacro(vtkKWSelectionFrame, "1.42");

//----------------------------------------------------------------------------
class vtkKWSelectionFrameInternals
{
public:
  typedef vtksys_stl::list<vtksys_stl::string> PoolType;
  typedef vtksys_stl::list<vtksys_stl::string>::iterator PoolIterator;

  PoolType Pool;
};

//----------------------------------------------------------------------------
vtkKWSelectionFrame::vtkKWSelectionFrame()
{
  this->Internals             = new vtkKWSelectionFrameInternals;

  this->TitleBar              = vtkKWFrame::New();
  this->Title                 = vtkKWLabel::New();
  this->SelectionList         = vtkKWMenuButton::New();
  this->CloseButton           = vtkKWPushButton::New();
  this->ToolbarSet            = vtkKWToolbarSet::New();
  this->TitleBarRightSubframe = vtkKWFrame::New();
  this->BodyFrame             = vtkKWFrame::New();

  this->CloseCommand          = NULL;
  this->SelectionListCommand  = NULL;
  this->SelectCommand         = NULL;
  this->DoubleClickCommand    = NULL;
  this->ChangeTitleCommand    = NULL;

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

  this->Selected           = 0;
  this->SelectionListVisibility  = 1;
  this->AllowClose       = 0;
  this->AllowChangeTitle = 0;
  this->ToolbarSetVisibility     = 0;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame::~vtkKWSelectionFrame()
{
  // Delete our pool

  delete this->Internals;

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

  if (this->CloseButton)
    {
    this->CloseButton->Delete();
    this->CloseButton = NULL;
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

  if (this->CloseCommand)
    {
    delete [] this->CloseCommand;
    this->CloseCommand = NULL;
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

  if (this->DoubleClickCommand)
    {
    delete [] this->DoubleClickCommand;
    this->DoubleClickCommand = NULL;
    }

  if (this->ChangeTitleCommand)
    {
    delete [] this->ChangeTitleCommand;
    this->ChangeTitleCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->Script("%s config -bd 1 -relief ridge",  this->GetWidgetName());

  // The title bar

  this->TitleBar->SetParent(this);
  this->TitleBar->Create(app);

  // The selection button

  this->SelectionList->SetParent(this->TitleBar);
  this->SelectionList->Create(app);
  this->SelectionList->IndicatorOff();
  this->SelectionList->SetImageToPredefinedIcon(vtkKWIcon::IconExpand);

  // The close button

  this->CloseButton->SetParent(this->TitleBar);
  this->CloseButton->Create(app);
  this->CloseButton->SetImageToPredefinedIcon(vtkKWIcon::IconShrink);
  this->CloseButton->SetCommand(this, "CloseCallback");
  this->CloseButton->SetBalloonHelpString("Close window");

  // The title itself

  this->Title->SetParent(this->TitleBar);
  this->Title->Create(app);
  this->Title->SetJustificationToLeft();
  this->Title->SetAnchorToWest();
  this->Title->SetText("<Click to Select>");
  
  // The subframe on the right

  this->TitleBarRightSubframe->SetParent(this->TitleBar);
  this->TitleBarRightSubframe->Create(app);

  // The toobar

  this->ToolbarSet->SetParent(this);
  this->ToolbarSet->BottomSeparatorVisibilityOff();
  this->ToolbarSet->Create(app);

  // The body frame

  this->BodyFrame->SetParent(this);
  this->BodyFrame->Create(app);
  this->BodyFrame->SetBackgroundColor(0.0, 0.0, 0.0);

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
    this->TitleBar->UnpackChildren();
    tk_cmd << "pack " << this->TitleBar->GetWidgetName()
           << " -side top -fill x -expand n" << endl;
    }

  if (this->SelectionListVisibility && this->SelectionList->IsCreated())
    {
    tk_cmd << "pack " << this->SelectionList->GetWidgetName()
           << " -side left -anchor w -fill y -padx 1 -pady 1" << endl;
    }

  if (this->Title->IsCreated())
    {
    tk_cmd << "pack " << this->Title->GetWidgetName()
           << " -side left -anchor w -fill x -expand y" << endl;
    }
  
  if (this->TitleBarRightSubframe->IsCreated())
    {
    tk_cmd << "pack " << this->TitleBarRightSubframe->GetWidgetName()
           << " -side left -anchor e -padx 2 -fill x -expand n" << endl;
    }
  
  if (this->AllowClose && this->CloseButton->IsCreated())
    {
    tk_cmd << "pack " << this->CloseButton->GetWidgetName()
           << " -side left -anchor e -fill y -padx 1 -pady 1 ";
    tk_cmd << endl;
    }

  if (this->ToolbarSetVisibility && this->ToolbarSet->IsCreated())
    {
    tk_cmd << "pack " << this->ToolbarSet->GetWidgetName()
           << " -side top -fill x -expand no -padx 1 -pady 1" << endl;
    this->ToolbarSet->Pack();
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

  const int nb_widgets = 4;
  vtkKWWidget *widgets[nb_widgets] = 
    {
      this->TitleBar,
      this->SelectionList,
      this->ToolbarSet,
      this->Title
    };
      
  for (int i = 0; i < nb_widgets; i++)
    {
    if (widgets[i] && widgets[i]->IsCreated())
      {
      tk_cmd << "bind " << widgets[i]->GetWidgetName() 
             << " <ButtonPress-1> {" << this->GetTclName() 
             << " SelectCallback}" << endl;
      tk_cmd << "bind " << widgets[i]->GetWidgetName() 
             << " <Double-1> {" << this->GetTclName() 
             << " DoubleClickCallback}" << endl;
      }
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

  const int nb_widgets = 4;
  vtkKWWidget *widgets[nb_widgets] = 
    {
      this->TitleBar,
      this->SelectionList,
      this->ToolbarSet,
      this->Title
    };
      
  for (int i = 0; i < nb_widgets; i++)
    {
    if (widgets[i] && widgets[i]->IsCreated())
      {
      tk_cmd << "bind " << widgets[i]->GetWidgetName() 
             << " <ButtonPress-1> {}" << endl;
      tk_cmd << "bind " << widgets[i]->GetWidgetName() 
             << " <Double-1> {}" << endl;
      }
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitle(const char *title)
{
  this->Title->SetText(title);
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrame::GetTitle()
{
  return this->Title->GetText();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrame::SetColor(
  double *color, double r, double g, double b)
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
  double r, double g, double b)
{
  if (this->SetColor(this->TitleColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleSelectedColor(
  double r, double g, double b)
{
  if (this->SetColor(this->TitleSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundColor(
  double r, double g, double b)
{
  if (this->SetColor(this->TitleBackgroundColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundSelectedColor(
  double r, double g, double b)
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
void vtkKWSelectionFrame::SetSelectionListVisibility(int arg)
{
  if (this->SelectionListVisibility == arg)
    {
    return;
    }

  this->SelectionListVisibility = arg;

  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetAllowClose(int arg)
{
  if (this->AllowClose == arg)
    {
    return;
    }

  this->AllowClose = arg;

  this->Modified();
  this->Pack();
  this->UpdateSelectionList();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetAllowChangeTitle(int arg)
{
  if (this->AllowChangeTitle == arg)
    {
    return;
    }

  this->AllowChangeTitle = arg;

  this->Modified();
  this->UpdateSelectionList();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetToolbarSetVisibility(int arg)
{
  if (this->ToolbarSetVisibility == arg)
    {
    return;
    }

  this->ToolbarSetVisibility = arg;

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

  double *fgcolor, *bgcolor;

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
  this->Internals->Pool.clear();
  
  for (int i = 0; i < num; i++)
    {
    this->Internals->Pool.push_back(list[i]);
    }

  this->UpdateSelectionList();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateSelectionList()
{
  if (!this->SelectionList->IsCreated())
    {
    return;
    }

  vtksys_stl::string callback;

  vtkKWMenu *menu = this->SelectionList->GetMenu();
  menu->DeleteAllMenuItems();
  
  vtkKWSelectionFrameInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    callback = "SelectionListCallback {";
    callback += *it;
    callback += "}";
    menu->AddCommand((*it).c_str(), this, callback.c_str());
    }

  // Add more commands

  if (this->AllowClose || this->AllowChangeTitle)
    {
    if (this->Internals->Pool.size())
      {
      menu->AddSeparator();
      }
    if (this->AllowChangeTitle)
      {
      menu->AddCommand(
        "Change Title", this, "ChangeTitleCallback", "Change frame title");
      }
    if (this->AllowClose)
      {
      menu->AddCommand(
        "Close", this, "CloseCallback", "Close frame");
      }
    }

  // The selection list is disabled when there are no entries

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectionListCommand(vtkObject *object,
                                                  const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionListCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetCloseCommand(vtkObject *object,
                                          const char *method)
{
  this->SetObjectMethodCommand(&this->CloseCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectCommand(vtkObject *object,
                                           const char *method)
{
  this->SetObjectMethodCommand(&this->SelectCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetDoubleClickCommand(vtkObject *object,
                                                const char *method)
{
  this->SetObjectMethodCommand(&this->DoubleClickCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetChangeTitleCommand(vtkObject *object,
                                                const char *method)
{
  this->SetObjectMethodCommand(&this->ChangeTitleCommand, object, method);
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
void vtkKWSelectionFrame::CloseCallback()
{
  if (this->CloseCommand)
    {
    this->Script("eval {%s %s}",
                 this->CloseCommand, this->GetTclName());
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
void vtkKWSelectionFrame::DoubleClickCallback()
{
  this->SelectCallback();

  if (this->DoubleClickCommand)
    {
    this->Script("eval {%s %s}",
                 this->DoubleClickCommand, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::ChangeTitleCallback()
{
  if (this->ChangeTitleCommand)
    {
    this->Script("eval {%s %s}",
                 this->ChangeTitleCommand, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->TitleBar);
  this->PropagateEnableState(this->SelectionList);
  this->PropagateEnableState(this->CloseButton);
  this->PropagateEnableState(this->Title);
  this->PropagateEnableState(this->TitleBarRightSubframe);
  this->PropagateEnableState(this->ToolbarSet);
  this->PropagateEnableState(this->BodyFrame);

  if (this->SelectionList &&
      this->SelectionList->GetMenu() &&
      !this->SelectionList->GetMenu()->GetNumberOfItems())
    {
    this->SelectionList->SetEnabled(0);
    }

  if (this->GetEnabled())
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
  os << indent << "CloseButton: " << this->CloseButton << endl;
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
  os << indent << "SelectionListVisibility: " << (this->SelectionListVisibility ? "On" : "Off") << endl;
  os << indent << "AllowClose: " << (this->AllowClose ? "On" : "Off") << endl;
  os << indent << "AllowChangeTitle: " << (this->AllowChangeTitle ? "On" : "Off") << endl;
  os << indent << "ToolbarSetVisibility: " << (this->ToolbarSetVisibility ? "On" : "Off") << endl;
}

