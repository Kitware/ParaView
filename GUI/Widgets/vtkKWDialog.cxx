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
#include "vtkKWTkUtilities.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWDialog );
vtkCxxRevisionMacro(vtkKWDialog, "1.57");

//----------------------------------------------------------------------------
vtkKWDialog::vtkKWDialog()
{
  this->Done = vtkKWDialog::StatusCanceled;
  this->Beep = 0;
  this->BeepType = 0;
  this->Modal = 1;
}

//----------------------------------------------------------------------------
int vtkKWDialog::PreInvoke()
{
  this->Done = vtkKWDialog::StatusActive;

  if (!this->IsMapped())
    {
    this->GetApplication()->RegisterDialogUp(this);
    this->Display();
    }

  if (this->Beep)
    {
    vtkKWTkUtilities::Bell(this->GetApplication());
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWDialog::PostInvoke()
{
  if (this->IsMapped())
    {
    this->Withdraw();
    this->GetApplication()->UnRegisterDialogUp(this);
    }
}

//----------------------------------------------------------------------------
int vtkKWDialog::IsUserDoneWithDialog()
{
  return this->Done != vtkKWDialog::StatusActive;
}

//----------------------------------------------------------------------------
int vtkKWDialog::Invoke()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  if (!this->PreInvoke())
    {
    return 0;
    }

  // Wait for the end

  while (!this->IsUserDoneWithDialog())
    {
    Tcl_DoOneEvent(0);    
    }

  this->PostInvoke();

  return (this->Done == vtkKWDialog::StatusCanceled ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWDialog::Display()
{
  this->Done = vtkKWDialog::StatusActive;
  this->Superclass::Display();
}

//----------------------------------------------------------------------------
void vtkKWDialog::Cancel()
{
  this->Done = vtkKWDialog::StatusCanceled;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::OK()
{
  this->Done = vtkKWDialog::StatusOK;  
}

//----------------------------------------------------------------------------
void vtkKWDialog::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  this->SetDeleteWindowProtocolCommand(this, "Cancel");
}

//----------------------------------------------------------------------------
void vtkKWDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Beep: " << this->GetBeep() << endl;
  os << indent << "BeepType: " << this->GetBeepType() << endl;
}

