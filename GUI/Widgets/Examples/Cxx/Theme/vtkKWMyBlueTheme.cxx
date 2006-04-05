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
#include "vtkKWTkOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMyBlueTheme);
vtkCxxRevisionMacro(vtkKWMyBlueTheme, "1.1");

//----------------------------------------------------------------------------
void vtkKWMyBlueTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  double bgcolor[3] = {0.2, 0.3, 0.8};
  //double bgcolor[3] = {0.79, 0.784, 0.831};
  //double bgcolor[3] = {0.737, 0.831, 0.737};
  double ltbgcolor[3] = {0.8, 0.8, 0.1};

  odb->AddEntryAsDouble3("*", "BackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("*", "ActiveBackgroundColor", bgcolor);

  odb->AddEntryAsDouble3("vtkKWEntry", "DisabledBackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWEntry", "ReadOnlyBackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWSpinBox", "DisabledBackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWSpinBox", "ReadOnlyBackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWSpinBox", "ButtonBackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWScale", "TroughColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWScrollbar", "TroughColor", bgcolor);
  odb->AddEntryAsDouble3(
    "vtkKWMultiColumnList", "ColumnLabelBackgroundColor", bgcolor);

  odb->AddEntryAsDouble3("*", "ActiveForegroundColor", 0.6, 0.3, 0.3);
  odb->AddEntryAsDouble3("*", "DisabledForegroundColor", 0.1, 0.1, 0.3);

  odb->AddEntryAsDouble3("vtkKWCheckButton", "SelectColor", ltbgcolor);
  odb->AddEntryAsDouble3("vtkKWMenu", "SelectColor", ltbgcolor);
}

//----------------------------------------------------------------------------
void vtkKWMyBlueTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
