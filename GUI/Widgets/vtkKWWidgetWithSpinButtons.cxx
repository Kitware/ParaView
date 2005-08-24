/*=========================================================================

  Module:    vtkKWWidgetWithSpinButtons.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidgetWithSpinButtons.h"

#include "vtkKWSpinButtons.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWidgetWithSpinButtons);
vtkCxxRevisionMacro(vtkKWWidgetWithSpinButtons, "1.1");

//----------------------------------------------------------------------------
vtkKWWidgetWithSpinButtons::vtkKWWidgetWithSpinButtons()
{
  this->SpinButtons = vtkKWSpinButtons::New();
}

//----------------------------------------------------------------------------
vtkKWWidgetWithSpinButtons::~vtkKWWidgetWithSpinButtons()
{
  if (this->SpinButtons)
    {
    this->SpinButtons->Delete();
    this->SpinButtons = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithSpinButtons::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  // Create the spin buttons subwidget

  this->SpinButtons->SetParent(this);
  this->SpinButtons->Create(app);
  this->SpinButtons->SetIncrementCommand(this, "IncrementValueCallback");
  this->SpinButtons->SetDecrementCommand(this, "DecrementValueCallback");

  // Subclasses will call this->Pack() here. Not now.
  // this->Pack();

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithSpinButtons::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  
  this->PropagateEnableState(this->SpinButtons);
}

// ---------------------------------------------------------------------------
void vtkKWWidgetWithSpinButtons::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->SpinButtons)
    {
    this->SpinButtons->SetBalloonHelpString(string);
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithSpinButtons::PrintSelf(ostream& os, vtkIndent indent)
{
  if (this->SpinButtons)
    {
    os << endl;
    this->SpinButtons->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
