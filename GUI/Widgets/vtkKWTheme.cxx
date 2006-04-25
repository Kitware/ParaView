/*=========================================================================

  Module:    vtkKWTheme.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWTheme.h"

#include "vtkKWOptions.h"
#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWOptionDataBase.h"
#include "vtkKWOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWTheme);
vtkCxxRevisionMacro(vtkKWTheme, "1.3");

//----------------------------------------------------------------------------
void vtkKWTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  /* Here is the kind of thing you would probably do here */
  
  // we should save the options we are about to replace here and
  // restore them in Uninstall

#if 0
  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  odb->AddEntryAsInt("vtkKWLabel", "Relief", vtkKWOptions::ReliefRidge);
  odb->AddEntryAsInt("vtkKWLabel", "BorderWidth", 2);
  odb->AddEntryAsDouble3("vtkKWLabel", "BackgroundColor", 0.2, 0.3, 0.6);
  odb->AddEntryAsDouble3("vtkKWFrame", "BackgroundColor", 0.2, 0.3, 0.6);
#endif
}

//----------------------------------------------------------------------------
void vtkKWTheme::Uninstall()
{
  // previous options should be restored
}

//----------------------------------------------------------------------------
void vtkKWTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
