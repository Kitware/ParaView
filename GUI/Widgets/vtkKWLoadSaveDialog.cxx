/*=========================================================================

  Module:    vtkKWLoadSaveDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWLoadSaveDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLoadSaveDialog );
vtkCxxRevisionMacro(vtkKWLoadSaveDialog, "1.35");

vtkKWLoadSaveDialog::vtkKWLoadSaveDialog()
{
  this->Done             = 1;
  this->FileTypes        = NULL;
  this->InitialFileName  = NULL;
  this->Title            = NULL;
  this->FileName         = NULL;
  this->LastPath         = NULL;
  this->DefaultExtension = NULL;

  this->SaveDialog       = 0;
  this->ChooseDirectory  = 0;

  this->SetTitle("Open Text Document");
  this->SetFileTypes("{{Text Document} {.txt}}");
}

vtkKWLoadSaveDialog::~vtkKWLoadSaveDialog()
{
  this->SetFileTypes(NULL);
  this->SetInitialFileName(NULL);
  this->SetTitle(NULL);
  this->SetFileName(NULL);
  this->SetDefaultExtension(NULL);
  this->SetLastPath(NULL);
}

void vtkKWLoadSaveDialog::Create(vtkKWApplication *app, const char* /*args*/)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Nothing else here for now
}

int vtkKWLoadSaveDialog::Invoke()
{
  this->GetApplication()->SetDialogUp(1);
  ostrstream command;

  int support_choose_dir = this->GetApplication()->EvaluateBooleanExpression(
    "string equal [info commands tk_chooseDirectory] tk_chooseDirectory");

  if (this->ChooseDirectory && support_choose_dir)
    {
    command << "tk_chooseDirectory";
    }
  else
    {
    command << (this->SaveDialog ? "tk_getSaveFile" : "tk_getOpenFile");
    }

  command << " -title {" << (this->Title ? this->Title : "") << "}"
          << " -initialdir {" 
          << ((this->LastPath && strlen(this->LastPath)>0)? this->LastPath:".")
          << "}";

  if (this->ChooseDirectory)
    {
    if (support_choose_dir)
      {
      command << " -mustexist 1";
      }
    }
  else
    {
    command << " -defaultextension {" 
            << (this->DefaultExtension ? this->DefaultExtension : "") << "}"
#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 3)
            << " -initialfile {" 
            << (this->InitialFileName ? this->InitialFileName : "") << "}"
#endif
            << " -filetypes {" 
            << (this->FileTypes ? this->FileTypes : "") << "}";
    }
  
  vtkKWWindow* window = this->GetWindow();
  if (window)
    {
    command << " -parent " << window->GetWidgetName();
    }
  command << ends;
  const char *path = this->Script(command.str());
  command.rdbuf()->freeze(0);

  int res = 0;

  if (path && strlen(path))
    {
    path = this->ConvertTclStringToInternalString(path);
    
    this->SetFileName(path);
    if (this->ChooseDirectory && support_choose_dir)
      {
      this->SetLastPath(path);
      }
    else
      {
      this->GenerateLastPath(path);
      }
    res = 1;
    }
  else
    {
    this->SetFileName(0);
    }

  this->GetApplication()->SetDialogUp(0);
  this->Script("update");

  return res;
}

//----------------------------------------------------------------------------
const char* vtkKWLoadSaveDialog::GenerateLastPath(const char* path)
{
  this->SetLastPath(0);
  // Store last path
  if ( path && strlen(path) > 0 )
    {
    char *pth = kwsys::SystemTools::DuplicateString(path);
    int pos = strlen(path);
    // Strip off the file name
    while (pos && pth[pos] != '/' && pth[pos] != '\\')
      {
      pos--;
      }
    pth[pos] = '\0';
    // Store in the registery
    this->SetLastPath(pth);
    delete [] pth;
    }
  return this->LastPath;
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DefaultExtension: " << 
    (this->DefaultExtension?this->DefaultExtension:"none")
     << endl;
  os << indent << "FileName: " << (this->FileName?this->FileName:"none") 
     << endl;
  os << indent << "FileTypes: " << (this->FileTypes?this->FileTypes:"none") 
     << endl;
  os << indent << "InitialFileName: " 
     << (this->InitialFileName?this->InitialFileName:"none") 
     << endl;
  os << indent << "LastPath: " << (this->LastPath?this->LastPath:"none")
     << endl;
  os << indent << "SaveDialog: " << this->GetSaveDialog() << endl;
  os << indent << "ChooseDirectory: " << this->GetChooseDirectory() << endl;
  os << indent << "Title: " << (this->Title?this->Title:"none") << endl;
}

