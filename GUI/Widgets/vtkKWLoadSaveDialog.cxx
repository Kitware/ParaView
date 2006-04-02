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

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLoadSaveDialog );
vtkCxxRevisionMacro(vtkKWLoadSaveDialog, "1.51");

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
  this->NumberOfFileNames = 0;
  this->FileNames        = NULL;

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
  this->ResetFileNames();
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveDialog::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

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

  this->ResetFileNames();
  
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
      command << " -mustexist 1";
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

  if (this->MultipleSelection)
    {
    if (path && strlen(path) && strcmp(path, "{}") != 0)
      {
      int n = 0;
      CONST84 char **files = 0;

      if (Tcl_SplitList(this->GetApplication()->GetMainInterp(),
                        path, &n, &files) == TCL_OK)
        {
        if (n > 0)
          {
          this->NumberOfFileNames = n;
          this->FileNames = new char *[n];

          for (int i = 0; i < n; i++)
            {
            // do conversion twice since ConvertTclStringToInternalString
            //  creates a temporary char pointer
            int stringLength =
              strlen(this->ConvertTclStringToInternalString(files[i]));
            this->FileNames[i] = new char[stringLength + 1];
            strcpy(this->FileNames[i], 
                   this->ConvertTclStringToInternalString(files[i]));
            }

          this->GenerateLastPath(this->GetNthFileName(0));
          this->SetFileName(this->GetNthFileName(0));

          res = 1;
          }
        }
      }
    }
  else if (path && strlen(path))
    {
    this->SetFileName(this->ConvertTclStringToInternalString(path));

    this->NumberOfFileNames = 1;
    this->FileNames = new char *[1];
    this->FileNames[0] = new char[strlen(this->GetFileName()) + 1];
    strcpy(this->FileNames[0], this->GetFileName());

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


  if (res == 0)
    {
    this->SetFileName(0);
    }

  this->GetApplication()->UnRegisterDialogUp(this);

  // Calls to update are evil, document why this one is needed
  //this->Script ("update"); 

  this->Done = res + 1;

  return res;
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
void vtkKWLoadSaveDialog::ResetFileNames()
{
  // deallocate the list of files
  if (this->FileNames)
    {
    for(int i =0; i < this->NumberOfFileNames; i++)
      {          
      delete [] this->FileNames[i];
      }
    delete [] this->FileNames;
    }
  this->FileNames = 0;
  this->NumberOfFileNames = 0;
}

//----------------------------------------------------------------------------
const char *vtkKWLoadSaveDialog::GetNthFileName(int i)
{
  if (i < 0 || i >= this->NumberOfFileNames)
    {
    vtkErrorMacro(<< this->GetClassName()
                  << " index for GetFileName is out of range");
    return NULL;
    }

  return this->FileNames[i];
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
  os << indent << "FileNames: "
     << (this->NumberOfFileNames == 0 ? "none" : "") << endl;
  for (int i = 0; i < this->NumberOfFileNames; i++)
    {
    os << indent << "  " << this->GetNthFileName(i) << endl;
    }
}

