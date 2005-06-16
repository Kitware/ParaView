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
vtkCxxRevisionMacro(vtkKWDialog, "1.50");

int vtkKWDialogCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWDialog::vtkKWDialog()
{
  this->CommandFunction = vtkKWDialogCommand;
  this->Done = 1;
  this->Beep = 0;
  this->BeepType = 0;
  this->Modal = 1;
  this->DisplayPosition = vtkKWTopLevel::DisplayPositionMasterWindowCenter;
}

//----------------------------------------------------------------------------
int vtkKWDialog::Invoke()
{
  this->Done = 0;

  this->GetApplication()->RegisterDialogUp(this);

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
void vtkKWDialog::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->SetDeleteWindowProtocolCommand(this, "Cancel");
}

//----------------------------------------------------------------------------
void vtkKWDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Beep: " << this->GetBeep() << endl;
  os << indent << "BeepType: " << this->GetBeepType() << endl;
}

