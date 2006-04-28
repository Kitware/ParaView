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
vtkCxxRevisionMacro(vtkKWMyGreenTheme, "1.4");

//----------------------------------------------------------------------------
void vtkKWMyGreenTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  this->Superclass::Install();

  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  // Use this database convenience method to set all background color
  // options to a specific color.

  odb->AddBackgroundColorOptions(0.2, 0.8, 0.3);

  // Use this database convenience method to set all font options to
  // a specific font.

  odb->AddFontOptions("{Times 12}");

  // Customize a few more colors
  // Say, for example, the select color and active foreground.

  double ltbgcolor[3] = {0.8, 0.1, 0.8};

  odb->AddEntryAsDouble3(
    "vtkKWWidget", "SetActiveForegroundColor", ltbgcolor);
  odb->AddEntryAsDouble3(
    "vtkKWWidget", "SetDisabledForegroundColor", ltbgcolor);

  odb->AddEntryAsDouble3("vtkKWCheckButton", "SetSelectColor", ltbgcolor);
  odb->AddEntryAsDouble3("vtkKWMenu", "SetSelectColor", ltbgcolor);

  // Let's demonstrate the use of contexts. Here, I want any vtkKWPushButton
  // found inside a vtkKWMessageDialog to use a specific solid relief.

  odb->AddEntry(
    "vtkKWMessageDialog*vtkKWPushButton", "SetReliefToSolid", NULL);

  // I want *all* vtkKWPushButton to use a different background color.

  odb->AddEntryAsDouble3(
    "vtkKWPushButton", "SetBackgroundColor", 0.3, 0.92, 0.4);

  // Let's demonstrate the use of slots. Here, I want any vtkKWFrameWithLabel
  // to configure its CollapsibleFrame member to a specific solid relief.

  odb->AddEntry("vtkKWFrameWithLabel:CollapsibleFrame", 
                "SetReliefToSolid", NULL);
}

//----------------------------------------------------------------------------
void vtkKWMyGreenTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
