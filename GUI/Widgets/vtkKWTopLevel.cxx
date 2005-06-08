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
vtkCxxRevisionMacro(vtkKWTopLevel, "1.14");

int vtkKWTopLevelCommand(ClientData cd, Tcl_Interp *interp,
                         int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWTopLevel::vtkKWTopLevel()
{
  this->CommandFunction = vtkKWTopLevelCommand;
  this->Title           = NULL;
  this->WindowClass     = NULL;
  this->MasterWindow    = NULL;
  this->Menu            = NULL;
  this->HideDecoration  = 0;
  this->Modal           = 0;
  this->DisplayPosition = vtkKWTopLevel::DisplayPositionDefault;
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
void vtkKWTopLevel::Create(vtkKWApplication *app, const char *args)
{
  vtksys_stl::string str;
  if (this->GetWindowClass())
    {
    str += " -class ";
    str += this->GetWindowClass();
    str += " ";
    }
  else if (this->GetMasterWindow())
    {
    vtkKWTopLevel *master_top = 
      vtkKWTopLevel::SafeDownCast(this->GetMasterWindow());
    if (master_top && master_top->GetWindowClass())
      {
      str += " -class ";
      str += master_top->GetWindowClass();
      str += " ";
      }
    }
  str += " -visual best ";
  if (args)
    {
    str += args;
    }

  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::Create(app, "toplevel", str.c_str()))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
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

  if (this->DisplayPosition != vtkKWTopLevel::DisplayPositionDefault)
    {
    int x, y;
    this->ComputeDisplayPosition(&x, &y);
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
  if (this->IsCreated())
    {
    this->Script("wm withdraw %s", this->GetWidgetName());
    }
  if (this->Modal)
    {
    this->ReleaseGrab();
    }
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::ComputeDisplayPosition(int *x, int *y)
{
  if (!this->IsCreated() ||
      this->DisplayPosition == vtkKWTopLevel::DisplayPositionDefault)
    {
    return;
    }

  int width, height;

  if (this->DisplayPosition == vtkKWTopLevel::DisplayPositionPointer)
    {
    sscanf(this->Script("concat [winfo pointerx .] [winfo pointery .]"),
           "%d %d", x, y);
    }
  else if (this->DisplayPosition == 
           vtkKWTopLevel::DisplayPositionMasterWindowCenter ||
           this->DisplayPosition == 
           vtkKWTopLevel::DisplayPositionScreenCenter)
    {
    int sw, sh;
    sscanf(this->Script("concat [winfo screenwidth .] [winfo screenheight .]"),
           "%d %d", &sw, &sh);

    vtkKWTopLevel *master = 
      vtkKWTopLevel::SafeDownCast(this->GetMasterWindow());
    if (master && this->DisplayPosition == 
        vtkKWTopLevel::DisplayPositionMasterWindowCenter)
      {
      master->GetSize(&width, &height);
      master->GetPosition(x, y);
      
      *x += width / 2;
      *y += height / 2;
      
      if (*x > sw - 200)
        {
        *x = sw / 2;
        }
      if (*y > sh - 200)
        {
        *y = sh / 2;
        }
      }
    else
      {
      *x = sw / 2;
      *y = sh / 2;
      }
    }

  // That call is not necessary since it has been added to both
  // GetRequestedWidth and GetRequestedHeight. If it is removed from them
  // for performance reasons (I doubt it), uncomment that line.
  // The call to 'update' enable the geometry manager to compute the layout
  // of the widget behind the scene, and return proper values.
  // this->Script("update idletasks");

  width = this->GetRequestedWidth();
  height = this->GetRequestedHeight();

  if (*x > width / 2)
    {
    *x -= width / 2;
    }
  if (*y > height / 2)
    {
    *y -= height / 2;
    }
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
void vtkKWTopLevel::Raise()
{
  if (this->IsCreated())
    {
    this->Script("raise %s", this->GetWidgetName());
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
  this->Script("update idletasks");
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
  this->Script("update idletasks");
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
    this->Menu->Create(this->GetApplication(), NULL);

    this->Script("%s configure -menu %s", 
                 this->GetWidgetName(),
                 this->Menu->GetWidgetName());
    }

  return this->Menu;
}

//----------------------------------------------------------------------------
void vtkKWTopLevel::SetDeleteWindowProtocolCommand(
  vtkKWObject *obj, const char *command)
{ 
  if (this->IsCreated())
    {
    char *objcmd = NULL;
    this->SetObjectMethodCommand(&objcmd, obj, command);
    this->Script("wm protocol %s WM_DELETE_WINDOW {%s}",
                 this->GetWidgetName(), objcmd);
    delete [] objcmd;
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

