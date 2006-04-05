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
#include "vtkKWTkOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMyGreenTheme);
vtkCxxRevisionMacro(vtkKWMyGreenTheme, "1.1");

//----------------------------------------------------------------------------
void vtkKWMyGreenTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  double bgcolor[3] = {0.2, 0.8, 0.3};

  odb->AddEntryAsDouble3("*", "BackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("*", "ActiveBackgroundColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWMultiColumnList", "ColumnLabelBackgroundColor", bgcolor);

  odb->AddEntryAsDouble3("vtkKWScale", "TroughColor", bgcolor);
  odb->AddEntryAsDouble3("vtkKWScrollbar", "TroughColor", bgcolor);

  double ltbgcolor[3] = {0.8, 0.1, 0.8};

  odb->AddEntryAsDouble3("*", "ActiveForegroundColor", ltbgcolor);

  odb->AddEntryAsDouble3("vtkKWCheckButton", "SelectColor", ltbgcolor);
}

//----------------------------------------------------------------------------
void vtkKWMyGreenTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
