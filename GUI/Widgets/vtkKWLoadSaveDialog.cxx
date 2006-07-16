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
#include "vtkKWWindowBase.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLoadSaveDialog );
vtkCxxRevisionMacro(vtkKWLoadSaveDialog, "1.56");

//----------------------------------------------------------------------------
vtkKWLoadSaveDialog::vtkKWLoadSaveDialog()
{
  this->FileTypes        = NULL;
  this->InitialFileName  = NULL;
  this->FileName         = NULL;
  this->LastPath         = NULL;
  this->DefaultExtension = NULL;

  this->SaveDialog       = 0;
  this->ChooseDirectory  = 0;
  this->MultipleSelection = 0;
  this->FileNames        = vtkStringArray::New();

  this->SetTitle("Open Text Document");
  this->SetFileTypes("{{Text Document} {.txt}}");
}

//----------------------------------------------------------------------------
vtkKWLoadSaveDialog::~vtkKWLoadSaveDialog()
{
  this->SetFileTypes(NULL);
  this->SetInitialFileName(NULL);
  this->SetFileName(NULL);
  this->SetDefaultExtension(NULL);
  this->SetLastPath(NULL);
  if (this->FileNames)
    {
    this->FileNames->Delete();
    this->FileNames = 0;
    }
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveDialog::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // Note that for this specific class, we are not really going to
  // display or use the toplevel that has been just created, we
  // are going to use a native file browser, created in Invoke()
  // We also could have yanked out the whole code here, but this
  // is not critical since the toplevel is created in a hidden state
  // and I want to make sure that subclass can safely rely on
  // calling our Create() and expect the whole dialog to be created
  // correctly. So let's not break the creation chain.
}

//----------------------------------------------------------------------------
int vtkKWLoadSaveDialog::Invoke()
{
  this->GetApplication()->RegisterDialogUp(this);
  ostrstream command;

  this->FileNames->Reset();
  
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

  if (this->MultipleSelection)
    {
    command << " -multiple 1";
    }
  
  if (this->ChooseDirectory)
    {
    if (support_choose_dir)
      {
      command << " -mustexist 0";
      }
    }
  else
    {
    command << " -defaultextension {" 
            << (this->DefaultExtension ? this->DefaultExtension : "") << "}"
            << " -initialfile {" 
            << (this->InitialFileName ? this->InitialFileName : "") << "}"
            << " -filetypes {" 
            << (this->FileTypes ? this->FileTypes : "") << "}";
    }

  vtkKWWindowBase* window = vtkKWWindowBase::SafeDownCast(
    this->GetParentTopLevel());
  if (window)
    {
    command << " -parent " << window->GetWidgetName();
    }
  command << ends;
  const char *path = this->Script(command.str());
  command.rdbuf()->freeze(0);

  int res = 0;

  if (path && strlen(path) && strcmp(path, "{}") != 0)
    {
    if (this->MultipleSelection)
      {
      int n = 0;
      CONST84 char **files = 0;

      if (Tcl_SplitList(this->GetApplication()->GetMainInterp(),
                        path, &n, &files) == TCL_OK)
        {
        if (n > 0)
          {
          for (int i = 0; i < n; i++)
            {
            this->FileNames->InsertNextValue(
              this->ConvertTclStringToInternalString(files[i]));
            }

          this->GenerateLastPath(this->GetNthFileName(0));
          this->SetFileName(this->GetNthFileName(0));

          res = 1;
          }
        }
      }
    else
      {
      this->SetFileName(this->ConvertTclStringToInternalString(path));
      
      this->FileNames->InsertNextValue(this->GetFileName());
      
      if (this->ChooseDirectory && support_choose_dir)
        {
        this->SetLastPath(this->GetFileName());
        }
      else
        {
        this->GenerateLastPath(this->GetFileName());
        }
      
      res = 1;
      }
    }


  if (res == 0)
    {
    this->SetFileName(NULL);
    }

  this->GetApplication()->UnRegisterDialogUp(this);

  // Calls to update are evil, document why this one is needed
  //this->Script ("update"); 

  this->Done = res ? vtkKWDialog::StatusOK : vtkKWDialog::StatusCanceled;

  return (this->Done == vtkKWDialog::StatusCanceled ? 0 : 1);
}

//----------------------------------------------------------------------------
const char* vtkKWLoadSaveDialog::GenerateLastPath(const char* path)
{
  this->SetLastPath(0);
  // Store last path
  if ( path && strlen(path) > 0 )
    {
    char *pth = vtksys::SystemTools::DuplicateString(path);
    int pos = strlen(path);
    // Strip off the file name
    while (pos && pth[pos] != '/' && pth[pos] != '\\')
      {
      pos--;
      }
    pth[pos] = '\0';
    // Store in the registry
    this->SetLastPath(pth);
    delete [] pth;
    }
  return this->LastPath;
}

//----------------------------------------------------------------------------
int vtkKWLoadSaveDialog::GetNumberOfFileNames()
{
  return this->FileNames->GetNumberOfValues();
}

//----------------------------------------------------------------------------
const char *vtkKWLoadSaveDialog::GetNthFileName(int i)
{
  if (i < 0 || i >= this->FileNames->GetNumberOfValues())
    {
    vtkErrorMacro(<< this->GetClassName()
                  << " index for GetFileName is out of range");
    return NULL;
    }

  return this->FileNames->GetValue(i);
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveDialog::SaveLastPathToRegistry(const char* key)
{
  if (this->IsCreated() && this->GetLastPath())
    {
    this->GetApplication()->SetRegistryValue(
      1, "RunTime", key, this->GetLastPath());
    }
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveDialog::RetrieveLastPathFromRegistry(const char* key)
{
  if (this->IsCreated())
    {
    char buffer[1024];
    if (this->GetApplication()->GetRegistryValue(1, "RunTime", key, buffer) &&
        *buffer)
      {
      this->SetLastPath(buffer);
      }  
    }
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
  os << indent << "MultipleSelection: " << this->GetMultipleSelection() << endl;
  os << indent << "NumberOfFileNames: " << this->GetNumberOfFileNames() << endl;
  os << indent << "FileNames:  (" << this->GetFileNames() << ")\n";
  indent = indent.GetNextIndent();
  for(int i = 0; i < this->FileNames->GetNumberOfValues(); i++)
    {
    os << indent << this->FileNames->GetValue(i) << "\n";
    }
}

