/*=========================================================================

  Module:    vtkKWSimpleEntryDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSimpleEntryDialog.h"

#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWEntryLabeled.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro( vtkKWSimpleEntryDialog );
vtkCxxRevisionMacro(vtkKWSimpleEntryDialog, "1.7");

int vtkKWSimpleEntryDialogCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWSimpleEntryDialog::vtkKWSimpleEntryDialog()
{
  this->CommandFunction = vtkKWSimpleEntryDialogCommand;

  this->Entry = vtkKWEntryLabeled::New();
}

//----------------------------------------------------------------------------
vtkKWSimpleEntryDialog::~vtkKWSimpleEntryDialog()
{
  if (this->Entry)
    {
    this->Entry->Delete();
    this->Entry = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleEntryDialog::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("SimpleEntryDialog already created");
    return;
    }

  this->Superclass::Create(app, args);
  
  this->Entry->SetParent(this->MessageDialogFrame);
  this->Entry->Create(app, 0);

  this->Script("pack %s -side top -after %s -padx 4 -fill x -expand yes", 
               this->Entry->GetWidgetName(), this->Label->GetWidgetName());

  this->Script("bind %s <Return> {%s OK}",
               this->Entry->GetWidget()->GetWidgetName(), this->GetTclName());

  this->Script("bind %s <Escape> {%s Cancel}",
               this->Entry->GetWidget()->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
int vtkKWSimpleEntryDialog::Invoke()
{
  if (this->IsCreated())
    {
    this->Entry->GetWidget()->Focus();
    }

  return this->Superclass::Invoke();
}

//----------------------------------------------------------------------------
void vtkKWSimpleEntryDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Entry: " << this->Entry << endl;
}

