/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLoadSaveDialog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLoadSaveDialog );

vtkKWLoadSaveDialog::vtkKWLoadSaveDialog()
{
  this->Done = 1;
  this->FileTypes = 0;
  this->InitialDir = 0;
  this->Title = 0;
  this->FileName = 0;
  this->LastPath = 0;

  this->SaveDialog = 0;
  this->DefaultExt = 0;

  this->SetTitle("Open Text Document");
  this->SetFileTypes("{{Text Document} {.txt}}");
  this->SetInitialDir(".");
}

vtkKWLoadSaveDialog::~vtkKWLoadSaveDialog()
{
  this->SetFileTypes(0);
  this->SetInitialDir(0);
  this->SetTitle(0);
  this->SetFileName(0);
  this->SetDefaultExt(0);
  this->SetLastPath(0);
}

void vtkKWLoadSaveDialog::Create(vtkKWApplication *app, const char* /*args*/)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Load Save Dialog already created");
    return;
    }

  this->SetApplication(app);
  // Nothing else here for now
}

int vtkKWLoadSaveDialog::Invoke()
{
  this->Application->SetDialogUp(1);
  char *path = NULL;
  this->Script("%s -title \"%s\" -defaultextension {%s} "
	       "-filetypes {%s} -initialdir {%s}", 
	       (this->SaveDialog) ? "tk_getSaveFile" : "tk_getOpenFile", 
	       this->Title, this->DefaultExt ? this->DefaultExt : "",
	       this->FileTypes, this->InitialDir);
  path = this->Application->GetMainInterp()->result;
  if ( path && strlen(path) )
    {
    this->SetFileName(path);
    this->Application->SetDialogUp(0);
    return 1;
    }
  this->SetFileName(0);
  this->Application->SetDialogUp(0);
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DefaultExt: " << this->GetDefaultExt() << endl;
  os << indent << "FileName: " << this->GetFileName() << endl;
  os << indent << "FileTypes: " << this->GetFileTypes() << endl;
  os << indent << "InitialDir: " << this->GetInitialDir() << endl;
  os << indent << "LastPath: " << this->GetLastPath() << endl;
  os << indent << "SaveDialog: " << this->GetSaveDialog() << endl;
  os << indent << "Title: " << this->GetTitle() << endl;
}
