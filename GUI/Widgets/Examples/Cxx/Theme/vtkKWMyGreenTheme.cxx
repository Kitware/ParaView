/*=========================================================================

  Module:    vtkKWMyGreenTheme.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWMyGreenTheme.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWOptionDataBase.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMyGreenTheme);
vtkCxxRevisionMacro(vtkKWMyGreenTheme, "1.3");

//----------------------------------------------------------------------------
void vtkKWMyGreenTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  this->Superclass::Install();

  // Use the superclass convenience method to set all background color
  // options to a specific color

  this->SetBackgroundColorOptions(0.2, 0.8, 0.3);

  // Customize a few more colors

  double ltbgcolor[3] = {0.8, 0.1, 0.8};

  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  odb->AddEntryAsDouble3("*", "SetActiveForegroundColor", ltbgcolor);
  odb->AddEntryAsDouble3("*", "SetDisabledForegroundColor", ltbgcolor);

  odb->AddEntryAsDouble3("vtkKWCheckButton", "SetSelectColor", ltbgcolor);
  odb->AddEntryAsDouble3("vtkKWMenu", "SetSelectColor", ltbgcolor);

  odb->AddEntry("vtkKWPushButton", "SetReliefToSolid", NULL);

  odb->AddEntry("*", "SetFont", "{Times 12}");
}

//----------------------------------------------------------------------------
void vtkKWMyGreenTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
