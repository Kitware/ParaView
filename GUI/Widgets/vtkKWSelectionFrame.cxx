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
#include "vtkCallbackCommand.h"

#include <vtksys/stl/list>
#include <vtksys/stl/string>

vtkStandardNewMacro(vtkKWSelectionFrame);
vtkCxxRevisionMacro(vtkKWSelectionFrame, "1.49");

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

  this->CallbackCommand       = vtkCallbackCommand::New();
  
  this->OuterSelectionFrame   = vtkKWFrame::New();
  this->TitleBarFrame         = vtkKWFrame::New();
  this->Title                 = vtkKWLabel::New();
  this->Title->SetText("<Click to Select>");
  this->SelectionList         = vtkKWMenuButton::New();
  this->CloseButton           = vtkKWPushButton::New();
  this->BodyFrame             = vtkKWFrame::New();
  this->ToolbarSet            = NULL;
  this->LeftUserFrame         = NULL;
  this->RightUserFrame        = NULL;
  this->TitleBarUserFrame     = NULL;

  this->CloseCommand          = NULL;
  this->SelectionListCommand  = NULL;
  this->SelectCommand         = NULL;
  this->DoubleClickCommand    = NULL;
  this->ChangeTitleCommand    = NULL;
  this->TitleChangedCommand    = NULL;

  this->TitleColor[0]                   = 1.0;
  this->TitleColor[1]                   = 1.0;
  this->TitleColor[2]                   = 1.0;

  this->TitleSelectedColor[0]           = 1.0;
  this->TitleSelectedColor[1]           = 1.0;
  this->TitleSelectedColor[2]           = 1.0;

  this->TitleBackgroundColor[0]         = 0.6;
  this->TitleBackgroundColor[1]         = 0.6;
  this->TitleBackgroundColor[2]         = 0.6;

  this->TitleSelectedBackgroundColor[0] = 0.0;
  this->TitleSelectedBackgroundColor[1] = 0.0;
  this->TitleSelectedBackgroundColor[2] = 0.5;

  this->OuterSelectionFrameColor[0]     = 0.6;
  this->OuterSelectionFrameColor[1]     = 0.6;
  this->OuterSelectionFrameColor[2]     = 0.6;

  this->OuterSelectionFrameSelectedColor[0] = 1.0;
  this->OuterSelectionFrameSelectedColor[1] = 0.93;
  this->OuterSelectionFrameSelectedColor[2] = 0.79;

  this->Selected                 = 0;
  this->TitleBarVisibility       = 1;
  this->SelectionListVisibility  = 1;
  this->AllowClose               = 1;
  this->AllowChangeTitle         = 1;
  this->ToolbarSetVisibility     = 0;
  this->LeftUserFrameVisibility  = 0;
  this->RightUserFrameVisibility = 0;
  this->OuterSelectionFrameWidth = 0;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame::~vtkKWSelectionFrame()
{
  this->Close();

  // Delete our pool

  delete this->Internals;

  if (this->CallbackCommand)
    {
    this->RemoveCallbackCommandBindings();
    this->CallbackCommand->Delete();
    this->CallbackCommand = NULL;
    }

  if (this->OuterSelectionFrame)
    {
    this->OuterSelectionFrame->Delete();
    this->OuterSelectionFrame = NULL;
    }

  if (this->TitleBarFrame)
    {
    this->TitleBarFrame->Delete();
    this->TitleBarFrame = NULL;
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

  if (this->TitleBarUserFrame)
    {
    this->TitleBarUserFrame->Delete();
    this->TitleBarUserFrame = NULL;
    }

  if (this->ToolbarSet)
    {
    this->ToolbarSet->Delete();
    this->ToolbarSet = NULL;
    }

  if (this->LeftUserFrame)
    {
    this->LeftUserFrame->Delete();
    this->LeftUserFrame = NULL;
    }

  if (this->RightUserFrame)
    {
    this->RightUserFrame->Delete();
    this->RightUserFrame = NULL;
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

  if (this->TitleChangedCommand)
    {
    delete [] this->TitleChangedCommand;
    this->TitleChangedCommand = NULL;
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

  this->SetBorderWidth(1);
  this->SetReliefToRidge();

  // The outer selection frame

  this->OuterSelectionFrame->SetParent(this);
  this->OuterSelectionFrame->Create(app);
  this->OuterSelectionFrame->SetReliefToFlat();
  this->OuterSelectionFrame->SetBorderWidth(this->OuterSelectionFrameWidth);

  vtkKWWidget *parent = this->OuterSelectionFrame;

  // The title bar

  this->TitleBarFrame->SetParent(parent);
  this->TitleBarFrame->Create(app);

  // The selection button

  this->SelectionList->SetParent(parent);
  this->SelectionList->Create(app);
  this->SelectionList->IndicatorVisibilityOff();
  this->SelectionList->SetImageToPredefinedIcon(vtkKWIcon::IconExpand);

  // The close button

  this->CloseButton->SetParent(parent);
  this->CloseButton->Create(app);
  this->CloseButton->SetImageToPredefinedIcon(vtkKWIcon::IconShrink);
  this->CloseButton->SetCommand(this, "CloseCallback");
  this->CloseButton->SetBalloonHelpString("Close window");

  // The title itself

  this->Title->SetParent(this->TitleBarFrame);
  this->Title->Create(app);
  this->Title->SetJustificationToLeft();
  this->Title->SetAnchorToWest();
  
  // The body frame

  this->BodyFrame->SetParent(parent);
  this->BodyFrame->Create(app);
  this->BodyFrame->SetBackgroundColor(0.0, 0.0, 0.0);

  // Pack

  this->Pack();

  // Update aspect

  this->UpdateSelectedAspect();
  
  // Update enable state (this will Bind() too)

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
vtkKWToolbarSet* vtkKWSelectionFrame::GetToolbarSet()
{
  if (!this->ToolbarSet)
    {
    this->ToolbarSet = vtkKWToolbarSet::New();
    }

  if (!this->ToolbarSet->IsCreated() && this->IsCreated())
    {
    this->ToolbarSet->SetParent(this->OuterSelectionFrame);
    this->ToolbarSet->BottomSeparatorVisibilityOff();
    this->ToolbarSet->Create(this->GetApplication());
    this->Pack();
    this->UpdateEnableState();
    }

  return this->ToolbarSet;
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWSelectionFrame::GetTitleBarUserFrame()
{
  if (!this->TitleBarUserFrame)
    {
    this->TitleBarUserFrame = vtkKWFrame::New();
    }

  if (!this->TitleBarUserFrame->IsCreated() && this->IsCreated())
    {
    this->TitleBarUserFrame->SetParent(this->TitleBarFrame);
    this->TitleBarUserFrame->Create(this->GetApplication());
    this->Pack();
    this->UpdateEnableState();
    }

  return this->TitleBarUserFrame;
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWSelectionFrame::GetLeftUserFrame()
{
  if (!this->LeftUserFrame)
    {
    this->LeftUserFrame = vtkKWFrame::New();
    }

  if (!this->LeftUserFrame->IsCreated() && this->IsCreated())
    {
    this->LeftUserFrame->SetParent(this->OuterSelectionFrame);
    this->LeftUserFrame->Create(this->GetApplication());
    this->Pack();
    this->UpdateEnableState();
    }

  return this->LeftUserFrame;
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWSelectionFrame::GetRightUserFrame()
{
  if (!this->RightUserFrame)
    {
    this->RightUserFrame = vtkKWFrame::New();
    }

  if (!this->RightUserFrame->IsCreated() && this->IsCreated())
    {
    this->RightUserFrame->SetParent(this->OuterSelectionFrame);
    this->RightUserFrame->Create(this->GetApplication());
    this->Pack();
    this->UpdateEnableState();
    }

  return this->RightUserFrame;
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

  int has_list     = this->SelectionListVisibility;
  int has_close    = this->AllowClose;
  int has_titlebar = this->TitleBarVisibility;
  int has_toolbar  = this->ToolbarSetVisibility;

  int need_left = this->LeftUserFrameVisibility || 
    (this->SelectionListVisibility && !TitleBarVisibility);

  int need_right = this->RightUserFrameVisibility || 
    (this->AllowClose && !TitleBarVisibility);

  vtkKWWidget *parent = this->OuterSelectionFrame;

  tk_cmd 
    << "pack " << this->OuterSelectionFrame->GetWidgetName()
    << " -expand y -fill both -padx 0 -pady 0 -ipadx 0 -ipady 0" << endl;

  if (has_titlebar && this->TitleBarFrame->IsCreated())
    {
    this->TitleBarFrame->UnpackChildren();
    tk_cmd 
      << "grid " << this->TitleBarFrame->GetWidgetName()
      << " -column " << (need_left && has_list ? 1 : 0)
      << " -columnspan " 
      << (1+(need_left && has_list ? 0 : 1)+(need_right && has_close ? 0 : 1))
      << " -row 0 -ipadx 1 -ipady 1 -sticky news" << endl;
    }

  if (this->SelectionListVisibility && this->SelectionList->IsCreated())
    {
    if (need_left)
      {
      tk_cmd << "grid " << this->SelectionList->GetWidgetName()
             << " -column 0 -row 0 -sticky news -ipadx 1 -ipady 1"
             << " -in " << parent->GetWidgetName() << endl;
      }
    else
      {
      tk_cmd << "pack " << this->SelectionList->GetWidgetName()
             << " -side left -anchor w -fill y -ipadx 1 -ipady 1"
             << " -in " << this->TitleBarFrame->GetWidgetName() << endl;
      }
    }

  if (this->Title->IsCreated())
    {
    tk_cmd << "pack " << this->Title->GetWidgetName()
           << " -side left -anchor w -fill x -expand y" << endl;
    }
  
  if (this->TitleBarUserFrame && this->TitleBarUserFrame->IsCreated())
    {
    tk_cmd << "pack " << this->TitleBarUserFrame->GetWidgetName()
           << " -side left -anchor e -padx 2 -fill x -expand n" << endl;
    }
  
  if (this->AllowClose && this->CloseButton->IsCreated())
    {
    if (need_right)
      {
      tk_cmd << "grid " << this->CloseButton->GetWidgetName()
             << " -column 2 -row 0 -sticky news -ipadx 1 -ipady 1"
             << " -in " << parent->GetWidgetName() << endl;
      }
    else
      {
      tk_cmd << "pack " << this->CloseButton->GetWidgetName()
             << " -side left -anchor e -fill y -ipadx 1 -ipady 1 "
             << " -in " << this->TitleBarFrame->GetWidgetName() << endl;
      }
    }

  if (has_toolbar && this->ToolbarSet && this->ToolbarSet->IsCreated())
    {
    tk_cmd 
      << "grid " << this->ToolbarSet->GetWidgetName()
      << " -column " << (need_left && has_list ? 1 : 0)
      << " -columnspan " 
      << (1+(need_left && has_list ? 0 : 1)+(need_right && has_close ? 0 : 1))
      << " -row " << (has_titlebar ? 1 : 0)
      << " -sticky news -padx 0 -pady 0" << endl;
    this->ToolbarSet->Pack();
    }

  for (int i = 0; i < 2; i++)
    {
    tk_cmd << "grid columnconfig " << parent->GetWidgetName() << " " << i 
           << " -weight 0" << endl;
    tk_cmd << "grid rowconfig " << parent->GetWidgetName() << " " << i 
           << " -weight 0" << endl;
    }

  if (this->LeftUserFrameVisibility && 
      this->LeftUserFrame && this->LeftUserFrame->IsCreated())
    {
    tk_cmd << "grid " << this->LeftUserFrame->GetWidgetName()
           << " -column 0 " << " -row " << (has_titlebar || has_list ? 1 : 0)
           << " -rowspan 3 -sticky news -padx 0 -pady 0" << endl;
    }

  if (this->BodyFrame->IsCreated())
    {
    int row = ((has_titlebar ? 1 : 0) + (has_toolbar ? 1 : 0));

    tk_cmd 
      << "grid " << this->BodyFrame->GetWidgetName()
      << " -column " << (need_left ? 1 : 0)
      << " -columnspan "  << (1 + (need_left ? 0 : 1) + (need_right ? 0 : 1))
      << " -row " << row << " -rowspan 3 -sticky news -padx 0 -pady 0" << endl;
    tk_cmd << "grid columnconfig " << parent->GetWidgetName() << " " 
           << (need_left ? 1 : 0) << " -weight 1" << endl;
    if (row == 0 && !has_titlebar && !has_toolbar && (has_list || has_close))
      {
      row++; // we do not want to expand the row that has the buttons
      }
    tk_cmd << "grid rowconfig " << parent->GetWidgetName() << " " 
           << row << " -weight 1" << endl;
    }

  if (this->RightUserFrameVisibility && 
      this->RightUserFrame && this->RightUserFrame->IsCreated())
    {
    tk_cmd << "grid " << this->RightUserFrame->GetWidgetName()
           << " -column 2 " << " -row " << (has_titlebar || has_close ? 1 : 0)
           << " -rowspan 3 -sticky news -padx 0 -pady 0" << endl;
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

  this->AddCallbackCommandBindings();

  vtkKWWidget *widgets_b[] = 
    {
      this->OuterSelectionFrame,
      this->TitleBarFrame,
      this->Title,
      this->SelectionList,
      this->CloseButton,
      this->BodyFrame,
      this->ToolbarSet,
      this->LeftUserFrame,
      this->RightUserFrame,
      this->TitleBarUserFrame
    };
  vtkKWWidget *widgets_db[] = 
    {
      this->OuterSelectionFrame,
      this->TitleBarFrame,
      this->Title
    };
      
  size_t i;
  for (i = 0; i < (sizeof(widgets_b) / sizeof(widgets_b[0])); i++)
    {
    if (widgets_b[i])
      {
      widgets_b[i]->SetBinding("<ButtonPress-1>", this, "SelectCallback");
      }
    }
  for (i = 0; i < (sizeof(widgets_db) / sizeof(widgets_db[0])); i++)
    {
    if (widgets_db[i])
      {
      widgets_db[i]->SetBinding("<Double-1>", this, "DoubleClickCallback");
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UnBind()
{
  if (!this->IsAlive())
    {
    return;
    }

  this->RemoveCallbackCommandBindings();

  vtkKWWidget *widgets_b[] = 
    {
      this->OuterSelectionFrame,
      this->TitleBarFrame,
      this->Title,
      this->SelectionList,
      this->CloseButton,
      this->BodyFrame,
      this->ToolbarSet,
      this->LeftUserFrame,
      this->RightUserFrame,
      this->TitleBarUserFrame
    };
  vtkKWWidget *widgets_db[] = 
    {
      this->OuterSelectionFrame,
      this->TitleBarFrame,
      this->Title
    };
      
  size_t i;
  for (i = 0; i < (sizeof(widgets_b) / sizeof(widgets_b[0])); i++)
    {
    if (widgets_b[i])
      {
      widgets_b[i]->RemoveBinding("<ButtonPress-1>");
      }
    }
  for (i = 0; i < (sizeof(widgets_db) / sizeof(widgets_db[0])); i++)
    {
    if (widgets_db[i])
      {
      widgets_db[i]->RemoveBinding("<Double-1>");
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::AddCallbackCommandBindings()
{
  if (this->CallbackCommand)
    {
    this->CallbackCommand->SetClientData(this); 
    this->CallbackCommand->SetCallback(vtkKWSelectionFrame::ProcessEventFunction);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::RemoveCallbackCommandBindings()
{
  if (this->CallbackCommand)
    {
    this->CallbackCommand->SetClientData(NULL); 
    this->CallbackCommand->SetCallback(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::ProcessEventFunction(
  vtkObject *object,
  unsigned long event,
  void *clientdata,
  void *calldata)
{
  // Pass to the virtual ProcessEvent method

  vtkKWSelectionFrame* self = static_cast<vtkKWSelectionFrame*>(clientdata);
  if (self)
    {
    self->ProcessEvent(object, event, calldata);
    }
}
  
//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitle(const char *title)
{
  if (this->Title)
    {
    vtksys_stl::string old_title(this->GetTitle());
    this->Title->SetText(title);
    if (strcmp(old_title.c_str(), this->GetTitle()))
      {
      if (this->TitleChangedCommand && *this->TitleChangedCommand && 
          this->IsCreated())
        {
        this->Script("eval {%s %s}",
                     this->TitleChangedCommand, this->GetTclName());
        }
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrame::GetTitle()
{
  if (this->Title)
    {
    return this->Title->GetText();
    }
  return NULL;
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
    this->UpdateSelectedAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleSelectedColor(
  double r, double g, double b)
{
  if (this->SetColor(this->TitleSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateSelectedAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundColor(
  double r, double g, double b)
{
  if (this->SetColor(this->TitleBackgroundColor, r, g, b))
    {
    this->Modified();
    this->UpdateSelectedAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleSelectedBackgroundColor(
  double r, double g, double b)
{
  if (this->SetColor(this->TitleSelectedBackgroundColor, r, g, b))
    {
    this->Modified();
    this->UpdateSelectedAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetOuterSelectionFrameColor(
  double r, double g, double b)
{
  if (this->SetColor(this->OuterSelectionFrameColor, r, g, b))
    {
    this->Modified();
    this->UpdateSelectedAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetOuterSelectionFrameSelectedColor(
  double r, double g, double b)
{
  if (this->SetColor(this->OuterSelectionFrameSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateSelectedAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetOuterSelectionFrameWidth(int arg)
{
  if (this->OuterSelectionFrameWidth == arg)
    {
    return;
    }

  this->OuterSelectionFrameWidth = arg;

  this->Modified();
  this->UpdateSelectedAspect();
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
  this->UpdateSelectedAspect();
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
void vtkKWSelectionFrame::SetLeftUserFrameVisibility(int arg)
{
  if (this->LeftUserFrameVisibility == arg)
    {
    return;
    }

  this->LeftUserFrameVisibility = arg;

  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetRightUserFrameVisibility(int arg)
{
  if (this->RightUserFrameVisibility == arg)
    {
    return;
    }

  this->RightUserFrameVisibility = arg;

  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBarVisibility(int arg)
{
  if (this->TitleBarVisibility == arg)
    {
    return;
    }

  this->TitleBarVisibility = arg;

  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateSelectedAspect()
{
  if (!this->IsCreated())
    {
    return;
    }

  double *title_fgcolor, *title_bgcolor, *selection_frame_bgcolor;

  if (this->Selected)
    {
    title_fgcolor = this->TitleSelectedColor;
    title_bgcolor = this->TitleSelectedBackgroundColor;
    selection_frame_bgcolor = this->OuterSelectionFrameSelectedColor;
    }
  else
    {
    title_fgcolor = this->TitleColor;
    title_bgcolor = this->TitleBackgroundColor;
    selection_frame_bgcolor = this->OuterSelectionFrameColor;
    }

  this->TitleBarFrame->SetBackgroundColor(
    title_bgcolor[0], title_bgcolor[1], title_bgcolor[2]);

  this->Title->SetBackgroundColor(
    title_bgcolor[0], title_bgcolor[1], title_bgcolor[2]);

  this->Title->SetForegroundColor(
    title_fgcolor[0], title_fgcolor[1], title_fgcolor[2]);

  if (this->TitleBarUserFrame)
    {
    this->TitleBarUserFrame->SetBackgroundColor(
      title_bgcolor[0], title_bgcolor[1], title_bgcolor[2]);
    }

  if (this->OuterSelectionFrame)
    {
    this->OuterSelectionFrame->SetBackgroundColor(selection_frame_bgcolor);
    this->OuterSelectionFrame->SetBorderWidth(this->OuterSelectionFrameWidth);
    }
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
void vtkKWSelectionFrame::SetTitleChangedCommand(vtkObject *object,
                                                const char *method)
{
  this->SetObjectMethodCommand(&this->TitleChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SelectionListCallback(const char *menuItem)
{
  if (this->SelectionListCommand && *this->SelectionListCommand && 
      this->IsCreated())
    {
    this->Script("eval {%s {%s} %s}",
                 this->SelectionListCommand, menuItem, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::CloseCallback()
{
  this->Close();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::Close()
{
  this->UnBind();

  if (this->CallbackCommand)
    {
    this->RemoveCallbackCommandBindings();
    this->CallbackCommand->Delete();
    this->CallbackCommand = NULL;
    }

  if (this->CloseCommand && *this->CloseCommand && this->IsCreated())
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

  if (this->SelectCommand && *this->SelectCommand && this->IsCreated())
    {
    this->Script("eval {%s %s}", this->SelectCommand, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::DoubleClickCallback()
{
  this->SelectCallback();

  if (this->DoubleClickCommand && *this->DoubleClickCommand)
    {
    this->Script("eval {%s %s}",
                 this->DoubleClickCommand, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::ChangeTitleCallback()
{
  if (this->ChangeTitleCommand && *this->ChangeTitleCommand && 
      this->IsCreated())
    {
    this->Script("eval {%s %s}",
                 this->ChangeTitleCommand, this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->OuterSelectionFrame);
  this->PropagateEnableState(this->TitleBarFrame);
  this->PropagateEnableState(this->SelectionList);
  this->PropagateEnableState(this->CloseButton);
  this->PropagateEnableState(this->Title);
  this->PropagateEnableState(this->TitleBarUserFrame);
  this->PropagateEnableState(this->ToolbarSet);
  this->PropagateEnableState(this->LeftUserFrame);
  this->PropagateEnableState(this->RightUserFrame);
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
  os << indent << "TitleBarUserFrame: " << this->TitleBarUserFrame
     << endl;
  os << indent << "SelectionList: " << this->SelectionList << endl;
  os << indent << "CloseButton: " << this->CloseButton << endl;
  os << indent << "ToolbarSet: " << this->ToolbarSet << endl;
  os << indent << "LeftUserFrame: " << this->LeftUserFrame << endl;
  os << indent << "RightUserFrame: " << this->RightUserFrame << endl;
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
  os << indent << "TitleSelectedBackgroundColor: ("
     << this->TitleSelectedBackgroundColor[0] << ", " 
     << this->TitleSelectedBackgroundColor[1] << ", " 
     << this->TitleSelectedBackgroundColor[2] << ")" << endl;
  os << indent << "OuterSelectionFrameColor: ("
     << this->OuterSelectionFrameColor[0] << ", " 
     << this->OuterSelectionFrameColor[1] << ", " 
     << this->OuterSelectionFrameColor[2] << ")" << endl;
  os << indent << "Selected: " << (this->Selected ? "On" : "Off") << endl;
  os << indent << "SelectionListVisibility: " << (this->SelectionListVisibility ? "On" : "Off") << endl;
  os << indent << "AllowClose: " << (this->AllowClose ? "On" : "Off") << endl;
  os << indent << "AllowChangeTitle: " << (this->AllowChangeTitle ? "On" : "Off") << endl;
  os << indent << "ToolbarSetVisibility: " << (this->ToolbarSetVisibility ? "On" : "Off") << endl;
  os << indent << "LeftUserFrameVisibility: " << (this->LeftUserFrameVisibility ? "On" : "Off") << endl;
  os << indent << "RightUserFrameVisibility: " << (this->RightUserFrameVisibility ? "On" : "Off") << endl;
  os << indent << "TitleBarVisibility: " << (this->TitleBarVisibility ? "On" : "Off") << endl;
  os << indent << "OuterSelectionFrameWidth: " << this->OuterSelectionFrameWidth << endl;
}

