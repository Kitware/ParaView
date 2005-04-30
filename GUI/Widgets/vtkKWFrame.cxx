/*=========================================================================

  Module:    vtkKWFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWFrame);
vtkCxxRevisionMacro(vtkKWFrame, "1.26");

//----------------------------------------------------------------------------
void vtkKWFrame::Create(vtkKWApplication *app, const char* args)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::Create(
        app, "frame", (args ? args : "-bd 0 -relief flat")))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWFrame::SetWidth(int width)
{
  if (this->IsCreated() && this->HasConfigurationOption("-width"))
    {
    this->Script("%s config -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWFrame::SetHeight(int height)
{
  if (this->IsCreated() && this->HasConfigurationOption("-height"))
    {
    this->Script("%s config -height %d", this->GetWidgetName(), height);
    }
}

//----------------------------------------------------------------------------
void vtkKWFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

