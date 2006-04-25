/*=========================================================================

  Module:    vtkKWMyBlueTheme.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWMyBlueTheme.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWOptionDataBase.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMyBlueTheme);
vtkCxxRevisionMacro(vtkKWMyBlueTheme, "1.3");

//----------------------------------------------------------------------------
void vtkKWMyBlueTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  this->Superclass::Install();

  // Use the superclass convenience method to set all background color
  // options to a specific color

  this->SetBackgroundColorOptions(0.2, 0.3, 0.8);

  // Customize a few more colors

  double ltbgcolor[3] = {0.8, 0.8, 0.1};

  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  odb->AddEntryAsDouble3("*", "SetActiveForegroundColor", 0.6, 0.3, 0.3);
  odb->AddEntryAsDouble3("*", "SetDisabledForegroundColor", 0.1, 0.1, 0.3);

  odb->AddEntryAsDouble3("vtkKWCheckButton", "SetSelectColor", ltbgcolor);
  odb->AddEntryAsDouble3("vtkKWMenu", "SetSelectColor", ltbgcolor);

  odb->AddEntry("vtkKWPushButton", "SetReliefToRidge", NULL);
  odb->AddEntry("vtkKWPushButton", "SetFont", "{Courier 10 bold}");
}

//----------------------------------------------------------------------------
void vtkKWMyBlueTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
