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
vtkCxxRevisionMacro(vtkKWMyBlueTheme, "1.4");

//----------------------------------------------------------------------------
void vtkKWMyBlueTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  this->Superclass::Install();

  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  // Use this database convenience method to set all background color
  // options to a specific color.

  odb->AddBackgroundColorOptions(0.2, 0.3, 0.8);

  // Customize a few more colors
  // Say, for example, the select color and active foreground.

  double ltbgcolor[3] = {0.8, 0.8, 0.1};

  odb->AddEntryAsDouble3(
    "vtkKWWidget", "SetActiveForegroundColor", 0.6, 0.3, 0.3);
  odb->AddEntryAsDouble3(
    "vtkKWWidget", "SetDisabledForegroundColor", 0.1, 0.1, 0.3);

  odb->AddEntryAsDouble3("vtkKWCheckButton", "SetSelectColor", ltbgcolor);
  odb->AddEntryAsDouble3("vtkKWMenu", "SetSelectColor", ltbgcolor);

  // I want *all* vtkKWPushButton to use a different relief

  odb->AddEntry("vtkKWPushButton", "SetReliefToRidge", NULL);

  // Let's demonstrate the use of contexts. Here, I want any vtkKWRadioButton
  // found inside a vtkKWRadioButtonSet to use a specific font.

  odb->AddEntry(
    "vtkKWRadioButtonSet*vtkKWRadioButton", "SetFont", "{Courier 10 bold}");

  // Let's demonstrate the use of slots. Here, I want any vtkKWFrameWithLabel
  // to configure its Label member to a specific color.

  odb->AddEntryAsDouble3(
    "vtkKWFrameWithLabel:Label", "SetBackgroundColor", 0.0, 0.0, 0.2);
  odb->AddEntryAsDouble3(
    "vtkKWFrameWithLabel:Label", "SetForegroundColor", 0.4, 0.6, 1.0);
}

//----------------------------------------------------------------------------
void vtkKWMyBlueTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
