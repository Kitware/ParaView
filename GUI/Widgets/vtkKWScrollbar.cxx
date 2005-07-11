/*=========================================================================

  Module:    vtkKWScrollbar.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWScrollbar.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWScrollbar);
vtkCxxRevisionMacro(vtkKWScrollbar, "1.2");

//----------------------------------------------------------------------------
void vtkKWScrollbar::Create(vtkKWApplication *app)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(app, "scrollbar"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetOrientation(int orientation)
{
  this->SetConfigurationOption(
    "-orient", vtkKWTkOptions::GetOrientationAsTkOptionValue(orientation));
}

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetOrientation()
{
  return vtkKWTkOptions::GetOrientationFromTkOptionValue(
    this->GetConfigurationOption("-orient"));
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetCommand(vtkObject *object, const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->SetConfigurationOption("-command", command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

