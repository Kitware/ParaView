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
vtkCxxRevisionMacro(vtkKWDialog, "1.45");

int vtkKWDialogCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWDialog::vtkKWDialog()
{
  this->CommandFunction = vtkKWDialogCommand;
  this->Done = 1;
  this->Beep = 0;
  this->BeepType = 0;
  this->InvokeAtPointer = 0;
  this->GrabDialog = 1;
}

//----------------------------------------------------------------------------
int vtkKWDialog::Invoke()
{
  this->Done = 0;

  this->GetApplication()->RegisterDialogUp(this);

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

    vtkKWTopLevel *master = 
      vtkKWTopLevel::SafeDownCast(this->GetMasterWindow());
    if (master)
      {
      master->GetSize(&width, &height);
      master->GetPosition(&x, &y);
      
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

  this->SetPosition(x, y);

  this->DeIconify();
  this->Raise();
  this->Focus();

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

  this->GetApplication()->UnRegisterDialogUp(this);

  return (this->Done-1);
}

//----------------------------------------------------------------------------
void vtkKWDialog::Display()
{
  this->Done = 0;

  this->Superclass::Display();

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
void vtkKWDialog::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app, args);

  this->Script("wm protocol %s WM_DELETE_WINDOW {%s Cancel}",
               this->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Beep: " << this->GetBeep() << endl;
  os << indent << "BeepType: " << this->GetBeepType() << endl;
  os << indent << "InvokeAtPointer: " << this->GetInvokeAtPointer() << endl;
}

