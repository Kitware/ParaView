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
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWDialog );
vtkCxxRevisionMacro(vtkKWDialog, "1.48");

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
  this->Modal = 1;
}

//----------------------------------------------------------------------------
void vtkKWDialog::ComputeInvokePosition(int *x, int *y)
{
  if (!this->IsCreated())
    {
    return;
    }

  int width, height;

  if (this->InvokeAtPointer)
    {
    sscanf(this->Script("concat [winfo pointerx .] [winfo pointery .]"),
           "%d %d", x, y);
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
int vtkKWDialog::Invoke()
{
  this->Done = 0;

  this->GetApplication()->RegisterDialogUp(this);

  int x, y;

  this->ComputeInvokePosition(&x, &y);
  this->SetPosition(x, y);

  this->Display();

  if (this->Beep)
    {
    this->Script("bell");
    }

  // Wait for the end

  while (!this->Done)
    {
    Tcl_DoOneEvent(0);    
    }

  this->Withdraw();

  this->GetApplication()->UnRegisterDialogUp(this);

  return (this->Done-1);
}

//----------------------------------------------------------------------------
void vtkKWDialog::Display()
{
  this->Done = 0;
  this->Superclass::Display();
}

//----------------------------------------------------------------------------
void vtkKWDialog::Cancel()
{
  this->Done = 1;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::OK()
{
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

  this->SetDeleteWindowProtocolCommand(this, "Cancel");
}

//----------------------------------------------------------------------------
void vtkKWDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Beep: " << this->GetBeep() << endl;
  os << indent << "BeepType: " << this->GetBeepType() << endl;
  os << indent << "InvokeAtPointer: " << this->GetInvokeAtPointer() << endl;
}

