/*=========================================================================

  Module:    vtkKWDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWDialog );
vtkCxxRevisionMacro(vtkKWDialog, "1.42");

int vtkKWDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWDialog::vtkKWDialog()
{
  this->CommandFunction = vtkKWDialogCommand;
  this->Done = 1;
  this->TitleString = 0;
  this->SetTitleString("Kitware Dialog");
  this->Beep = 0;
  this->BeepType = 0;
  this->MasterWindow = 0;
  this->InvokeAtPointer = 0;
  this->GrabDialog = 1;

  this->HasBeenMapped = 0;
}

//----------------------------------------------------------------------------
vtkKWDialog::~vtkKWDialog()
{
  this->SetTitleString(0);
  this->SetMasterWindow(0);
}

//----------------------------------------------------------------------------
int vtkKWDialog::Invoke()
{
  this->Done = 0;

  this->GetApplication()->SetDialogUp(1);

  int width, height;

  int x, y;

  if (this->InvokeAtPointer)
    {
    sscanf(this->Script("concat [winfo pointerx .] [winfo pointery .]"),
           "%d %d", &x, &y);
    }
  else
    {
    int sw, sh;
    sscanf(this->Script("concat [winfo screenwidth .] [winfo screenheight .]"),
           "%d %d", &sw, &sh);

    if (this->GetMasterWindow())
      {
      this->Script("wm geometry %s", this->GetMasterWindow()->GetWidgetName());
      sscanf(this->GetApplication()->GetMainInterp()->result, "%dx%d+%d+%d",
             &width, &height, &x, &y);

      x += width / 2;
      y += height / 2;

      if (x > sw - 200)
        {
        x = sw / 2;
        }
      if (y > sh - 200)
        {
        y = sh / 2;
        }
      }
    else
      {
      x = sw / 2;
      y = sh / 2;
      }
    }

  width = this->GetWidth();
  height = this->GetHeight();

  if (x > width / 2)
    {
    x -= width / 2;
    }
  if (y > height / 2)
    {
    y -= height / 2;
    }

  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), x, y);

  // map the window
  this->DeIconify();

  this->Script("focus %s",this->GetWidgetName());
  this->Script("update idletasks");
  if (this->GrabDialog)
    {
    this->Grab();
    }
  if ( this->Beep )
    {
    this->Script("bell");
    }

  // do a grab
  // wait for the end
  while (!this->Done)
    {
    Tcl_DoOneEvent(0);    
    }
  if (this->GrabDialog)
    {
    this->ReleaseGrab();
    }
  this->GetApplication()->SetDialogUp(0);
  return (this->Done-1);
}

//----------------------------------------------------------------------------
void vtkKWDialog::Display()
{
  this->Done = 0;

  // map the window
  this->DeIconify();
  this->Script("focus %s",this->GetWidgetName());
  this->Script("update idletasks");
  this->Grab();
}

//----------------------------------------------------------------------------
void vtkKWDialog::Cancel()
{
  this->Withdraw();
  this->ReleaseGrab();

  this->Done = 1;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::OK()
{
  this->Withdraw();
  this->ReleaseGrab();
  this->Done = 2;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::DeIconify()
{
  if (this->IsCreated())
    {
    this->Script("wm deiconify %s", this->GetWidgetName());
    this->HasBeenMapped = 1;
    }
}

//----------------------------------------------------------------------------
void vtkKWDialog::Withdraw()
{
  if (this->IsCreated())
    {
    this->Script("wm withdraw %s", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWDialog::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  const char *wname = this->GetWidgetName();
  if (this->MasterWindow)
    {
    this->Script("toplevel %s -class %s %s",
                 wname,
                 this->MasterWindow->GetWindowClass(),
                 (args ? args : ""));
    }
  else
    {
    this->Script("toplevel %s %s" ,wname, (args ? args : ""));
    }

  this->Script("wm iconname %s \"Dialog\"", wname);
  this->Script("wm protocol %s WM_DELETE_WINDOW {%s Cancel}",
               wname, this->GetTclName());

  this->SetTitle(this->TitleString);

  this->Withdraw();

  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
vtkKWWindow *vtkKWDialog::GetMasterWindow()
{
  return this->MasterWindow;
}

//----------------------------------------------------------------------------
void vtkKWDialog::SetMasterWindow(vtkKWWindow* win)
{
  if (this->MasterWindow != win) 
    { 
    this->MasterWindow = win; 
    if (this->MasterWindow) 
      { 
      if (this->IsCreated())
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    }   
}

//----------------------------------------------------------------------------
void vtkKWDialog::SetTitle( const char* title )
{
  if (this->IsCreated())
    {
    this->Script("wm title %s \"%s\"", this->GetWidgetName(), 
                 title);
    }
  else
    {
    this->SetTitleString(title);
    }
}

//----------------------------------------------------------------------------
int vtkKWDialog::GetWidth()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo reqwidth %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWDialog::GetHeight()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo reqheight %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Beep: " << this->GetBeep() << endl;
  os << indent << "BeepType: " << this->GetBeepType() << endl;
  os << indent << "InvokeAtPointer: " << this->GetInvokeAtPointer() << endl;
  os << indent << "HasBeenMapped: " << this->GetHasBeenMapped() << endl;
}

