/*=========================================================================

  Module:    vtkKWTopLevel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWTopLevel.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWMenu.h"

#include <vtksys/stl/string>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTopLevel );
vtkCxxRevisionMacro(vtkKWTopLevel, "1.26");

//----------------------------------------------------------------------------
vtkKWTopLevel::vtkKWTopLevel()
{
  this->Title           = NULL;
  this->WindowClass     = NULL;
  this->MasterWindow    = NULL;
  this->Menu            = NULL;
  this->HideDecoration  = 0;
  this->Modal           = 0;
  this->DisplayPosition = 
    vtkKWTopLevel::DisplayPositionMasterWindowCenterFirst;
}

//----------------------------------------------------------------------------
vtkKWTopLevel::~vtkKWTopLevel()
{
  this->SetTitle(NULL);
  this->SetMasterWindow(NULL);
  this->SetWindowClass(0);

  if (this->Menu)
    {
    this->Menu->Delete();
    this->Menu = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWApplication* vtkKWTopLevel::GetApplication()
{
  if (!this->Superclass::GetApplication() &&
      this->MasterWindow && this->MasterWindow->GetApplication())
    {
    this->SetApplication(this->MasterWindow->GetApplication());
    }

  return this->Superclass::GetApplication();
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::Create()
{
  vtksys_stl::string opts;

  if (this->GetWindowClass())
    {
    opts += " -class {";
    opts += this->GetWindowClass();
    opts += "} ";
    }
  else if (this->GetMasterWindow())
    {
    vtkKWTopLevel *master_top = 
      vtkKWTopLevel::SafeDownCast(this->GetMasterWindow());
    if (master_top && master_top->GetWindowClass())
      {
      opts += " -class {";
      opts += master_top->GetWindowClass();
      opts += "} ";
      }
    }
  opts += " -visual best -highlightthickness 0";

  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget("toplevel", opts.c_str()))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->PostCreate();
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::PostCreate()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Withdraw();

  this->Script("wm protocol %s WM_DELETE_WINDOW {%s Withdraw}",
               this->GetWidgetName(), this->GetTclName());
  
  if (this->GetTitle())
    {
    this->Script("wm title %s \"%s\"", 
                 this->GetWidgetName(), this->GetTitle());
    }

  if (this->GetMasterWindow() && this->GetMasterWindow()->IsCreated())
    {
    this->Script("wm transient %s [winfo toplevel %s]", 
                 this->GetWidgetName(), 
                 this->GetMasterWindow()->GetWidgetName());
    }

  if (this->HideDecoration)
    {
    this->Script("wm overrideredirect %s %d", 
                 this->GetWidgetName(), this->HideDecoration ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::Display()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Position the toplevel.

  int x, y;
  if (this->DisplayPosition != vtkKWTopLevel::DisplayPositionDefault &&
      this->ComputeDisplayPosition(&x, &y))
    {
    this->SetPosition(x, y);
    }

  this->DeIconify();
  this->Raise();

  this->Focus();

  if (this->Modal)
    {
    this->Grab();
    }
 }

//----------------------------------------------------------------------------
void vtkKWTopLevel::Withdraw()
{
  vtkKWTkUtilities::WithdrawTopLevel(this);
  if (this->Modal)
    {
    this->ReleaseGrab();
    }
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::ComputeDisplayPosition(int *x, int *y)
{
  if (!this->IsCreated() ||
      this->DisplayPosition == vtkKWTopLevel::DisplayPositionDefault)
    {
    return 0;
    }

  int display_pos = this->DisplayPosition;

  int width = this->GetWidth();
  int reqwidth = this->GetRequestedWidth();
  
  int height = this->GetHeight();
  int reqheight = this->GetRequestedHeight();

  int minwidth, minheight;
  this->GetMinimumSize(&minwidth, &minheight);

  // If we requested to center only the first time we display the toplevel,
  // check its current size. If it is down to a single pixel width or height,
  // then it has never been mapped, and we can try to center it. Otherwise just
  // use the current position

  if (display_pos == vtkKWTopLevel::DisplayPositionMasterWindowCenterFirst ||
      display_pos == vtkKWTopLevel::DisplayPositionScreenCenterFirst)
    {
    if (width == 1 || height == 1 || reqwidth == 1 || reqheight == 1)
      {
      if (display_pos == vtkKWTopLevel::DisplayPositionMasterWindowCenterFirst)
        {
        display_pos = vtkKWTopLevel::DisplayPositionMasterWindowCenter;
        }
      else
        {
        display_pos = vtkKWTopLevel::DisplayPositionScreenCenter;
        }
      }
    else
      {
      return 0;
      }
    }

  // Try to use the proper width

  if (reqwidth > width)
    {
    width = reqwidth;
    }
  if (minwidth > width)
    {
    width = minwidth;
    }

  if (reqheight > height)
    {
    height = reqheight;
    }
  if (minheight > height)
    {
    height = minheight;
    }

  // Display at pointer, or center

  if (display_pos == vtkKWTopLevel::DisplayPositionPointer)
    {
    vtkKWTkUtilities::GetMousePointerCoordinates(
      this->GetApplication()->GetMainInterp(), ".", x, y);
    }
  else if (display_pos == 
           vtkKWTopLevel::DisplayPositionMasterWindowCenter ||
           display_pos == 
           vtkKWTopLevel::DisplayPositionScreenCenter)
    {
    vtkKWTopLevel *master = 
      vtkKWTopLevel::SafeDownCast(this->GetMasterWindow());
    
    if (master && display_pos == 
        vtkKWTopLevel::DisplayPositionMasterWindowCenter)
      {
      int master_width, master_height;
      master->GetSize(&master_width, &master_height);
      master->GetPosition(x, y);
      
      *x += master_width / 2;
      *y += master_height / 2;
      }
    else
      {
      int screen_width, screen_height;
      vtkKWTkUtilities::GetScreenSize(
        this->GetApplication()->GetMainInterp(), ".", 
        &screen_width, &screen_height);

      *x = screen_width / 2;
      *y = screen_height / 2;
      }
    }

  // That call is not necessary since it has been added to both
  // GetRequestedWidth and GetRequestedHeight. If it is removed from them
  // for performance reasons (I doubt it), uncomment that line.
  // The call to 'update' enable the geometry manager to compute the layout
  // of the widget behind the scene, and return proper values.
  // this->Script("update idletasks");

  if (*x > width / 2)
    {
    *x -= width / 2;
    }
  if (*y > height / 2)
    {
    *y -= height / 2;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::DeIconify()
{
  if (this->IsCreated())
    {
    this->Script("wm deiconify %s", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetMasterWindow(vtkKWWidget* win)
{
  // Ref count it ?

  if (this->MasterWindow == win) 
    {
    return;
    }

  this->MasterWindow = win; 
  this->Modified(); 

  if (!this->GetApplication() && win->GetApplication())
    {
    this->SetApplication(win->GetApplication());
    }

  if (this->IsCreated() && 
      this->GetMasterWindow() && this->GetMasterWindow()->IsCreated())
    {
    this->Script("wm transient %s [winfo toplevel %s]", 
                 this->GetWidgetName(), 
                 this->GetMasterWindow()->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetHideDecoration(int arg)
{
  if (this->HideDecoration == arg) 
    {
    return;
    }

  this->HideDecoration = arg; 
  this->Modified(); 

  if (this->IsCreated())
    {
    this->Script("wm overrideredirect %s %d", 
                 this->GetWidgetName(), this->HideDecoration ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetTitle(const char* _arg)
{
  if (this->Title == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Title && _arg && (!strcmp(this->Title, _arg))) 
    {
    return;
    }

  if (this->Title) 
    { 
    delete [] this->Title; 
    }

  if (_arg)
    {
    this->Title = new char[strlen(_arg) + 1];
    strcpy(this->Title, _arg);
    }
  else
    {
    this->Title = NULL;
    }

  this->Modified();

  if (this->Title && this->IsCreated())
    {
    this->Script("wm title %s \"%s\"", this->GetWidgetName(), this->Title);
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetTitleToTopLevelTitle(vtkKWWidget *widget)
{
  if (widget && widget->IsCreated())
    {
    this->SetTitle(
      this->Script("wm title [winfo toplevel %s]", widget->GetWidgetName()));
    }
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetWidth()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo width %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetHeight()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo height %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetRequestedWidth()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // The call to 'update' enable the geometry manager to compute the layout
  // of the widget behind the scene, and return proper values.
  this->GetApplication()->ProcessIdleTasks();
  return atoi(this->Script("winfo reqwidth %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetRequestedHeight()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // The call to 'update' enable the geometry manager to compute the layout
  // of the widget behind the scene, and return proper values.
  this->GetApplication()->ProcessIdleTasks();
  return atoi(this->Script("winfo reqheight %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::SetPosition(int x, int y)
{
  if (!this->IsCreated())
    {
    return 0;
    }
  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), x, y);
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetPosition(int *x, int *y)
{
  return vtkKWTkUtilities::GetGeometry(this, NULL, NULL, x, y);
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::SetSize(int w, int h)
{
  if (!this->IsCreated())
    {
    return 0;
    }
  this->Script("wm geometry %s %dx%d", this->GetWidgetName(), w, h);
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetSize(int *w, int *h)
{
  if (!w || !h)
    {
    return 0;
    }
  *w = this->GetWidth();
  *h = this->GetHeight();
  return 1;
  //return vtkKWTkUtilities::GetGeometry(this, w, h, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::SetMinimumSize(int w, int h)
{
  if (!this->IsCreated())
    {
    return 0;
    }
  this->Script("wm minsize %s %d %d", this->GetWidgetName(), w, h);
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetMinimumSize(int *w, int *h)
{
  if (!this->IsCreated() || !w || !h)
    {
    return 0;
    }
  return sscanf(this->Script("wm minsize %s", this->GetWidgetName()), 
                "%d %d", w, h) == 2 ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::SetGeometry(const char *geometry)
{
  if (!this->IsCreated() || !geometry)
    {
    return 0;
    }
  this->Script("wm geometry %s {%s}", this->GetWidgetName(), geometry);
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkKWTopLevel::GetGeometry()
{
  if (!this->IsCreated())
    {
    return NULL;
    }
  return this->Script("wm geometry %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWTopLevel::GetMenu()
{
  if (!this->Menu)
    {
    this->Menu = vtkKWMenu::New();
    }

  if (!this->Menu->IsCreated() && this->IsCreated())
    {
    this->Menu->SetParent(this);
    this->Menu->SetTearOff(0);
    this->Menu->Create();
    this->SetConfigurationOption("-menu", this->Menu->GetWidgetName());
    }

  return this->Menu;
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetDeleteWindowProtocolCommand(
  vtkObject *object, const char *method)
{ 
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("wm protocol %s WM_DELETE_WINDOW {%s}",
                 this->GetWidgetName(), command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetIconName(const char *name)
{ 
  if (this->IsCreated())
    {
    this->Script("wm iconname %s {%s}",
                 this->GetWidgetName(), name ? name : NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetResizable(int w, int h)
{
  if (this->IsCreated())
    {
    this->Script("wm resizable %s %d %d", this->GetWidgetName(), w, h);
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWTopLevel::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWTopLevel::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWTopLevel::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWTopLevel::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWTopLevel::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWTopLevel::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWTopLevel::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetPadY(int arg)
{
  this->SetConfigurationOptionAsInt("-pady", arg);
}

//----------------------------------------------------------------------------
int vtkKWTopLevel::GetPadY()
{
  return this->GetConfigurationOptionAsInt("-pady");
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Title: ";
  if (this->GetTitle())
    {
    os << this->GetTitle() << endl;
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "WindowClass: ";
  if (this->GetWindowClass())
    {
    os << this->GetWindowClass() << endl;
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "MasterWindow: " << this->GetMasterWindow() << endl;
  os << indent << "HideDecoration: " << (this->HideDecoration ? "On" : "Off" ) << endl;
  os << indent << "Modal: " << this->GetModal() << endl;
  os << indent << "DisplayPosition: " << this->GetDisplayPosition() << endl;
}

