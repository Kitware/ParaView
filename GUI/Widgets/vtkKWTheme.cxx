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
vtkCxxRevisionMacro(vtkKWTheme, "1.5");

//----------------------------------------------------------------------------
vtkKWTheme::vtkKWTheme()
{
  this->BackupOptionDataBase = NULL;
}

//----------------------------------------------------------------------------
vtkKWTheme::~vtkKWTheme()
{
  if (this->BackupOptionDataBase)
    {
    this->BackupOptionDataBase->Delete();
    this->BackupOptionDataBase = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWTheme::Install()
{
  if (!this->GetApplication())
    {
    return;
    }

  this->BackupCurrentOptionDataBase();

  /* Here is the kind of thing you could probably do here.
     Check the Themes example in the Examples/Cxx subdirectory.
   */
  
#if 0
  vtkKWOptionDataBase *odb = this->GetApplication()->GetOptionDataBase();

  odb->AddEntryAsInt("vtkKWLabel", "SetReliefToGroove");
  odb->AddEntryAsInt("vtkKWLabel", "SetBorderWidth", 2);
  odb->AddEntryAsDouble3("vtkKWLabel", "SetBackgroundColor", 0.2, 0.3, 0.6);
  odb->AddEntryAsDouble3("vtkKWFrame", "SetBackgroundColor", 0.2, 0.3, 0.6);
#endif
}

//----------------------------------------------------------------------------
void vtkKWTheme::Uninstall()
{
  this->RestorePreviousOptionDataBase();
}

//----------------------------------------------------------------------------
void vtkKWTheme::BackupCurrentOptionDataBase()
{
  if (!this->GetApplication())
    {
    return;
    }

  if (!this->BackupOptionDataBase)
    {
    this->BackupOptionDataBase = vtkKWOptionDataBase::New();
    }
  else if (this->BackupOptionDataBase->GetNumberOfEntries())
    {
    return;
    }

  this->BackupOptionDataBase->DeepCopy(
    this->GetApplication()->GetOptionDataBase());
}

//----------------------------------------------------------------------------
void vtkKWTheme::RestorePreviousOptionDataBase()
{
  if (!this->GetApplication() || !this->BackupOptionDataBase)
    {
    return;
    }

  this->GetApplication()->GetOptionDataBase()->DeepCopy(
    this->BackupOptionDataBase);
}

//----------------------------------------------------------------------------
void vtkKWTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
