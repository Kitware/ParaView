/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSaveImageDialog.cxx
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
#include "vtkKWSaveImageDialog.h"
#include "vtkObjectFactory.h"

#include "vtkKWMessageDialog.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSaveImageDialog );




int vtkKWSaveImageDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWSaveImageDialog::vtkKWSaveImageDialog()
{
  this->FileName = NULL;
  this->CommandFunction = vtkKWSaveImageDialogCommand;
}

// invoke the dialog
void vtkKWSaveImageDialog::Invoke()
{
  int done = 0;
  char *path = NULL;
  
  while (!done)
    {
    this->Script("tk_getSaveFile -title {Save As Image} -defaultextension {.bmp} -filetypes {{{Windows Bitmap} {.bmp}} {{JPEG Images} {.jpg}} {{PNG Images} {.png}} {{Binary PPM} {.ppm}} {{TIFF Images} {.tif}}}");
    if (path)
      {
      free(path);
      }
    path =  
      strcpy(new char[strlen(this->Application->GetMainInterp()->result)+1], 
	     this->Application->GetMainInterp()->result);
    if (strlen(path) == 0)
      {
      done = 1;
      }  
    else if (!strcmp(path + strlen(path) - 4,".bmp") ||
	     !strcmp(path + strlen(path) - 4,".tif") ||
	     !strcmp(path + strlen(path) - 4,".png") ||
	     !strcmp(path + strlen(path) - 4,".jpg") ||
	     !strcmp(path + strlen(path) - 4,".ppm"))
      {
      done = 1;
      }
    else
      {
      // unknown file extension
      vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
      dlg2->Create(this->Application,"");
      dlg2->SetText(
        "A valid file extension was not found.\\n"
	"Please use a .bmp, .ppm, or .tif file extension\\n"
	"when naming your file.");
      dlg2->SetOptions(dlg2->GetOptions() | vtkKWMessageDialog::ErrorIcon);
      dlg2->SetTitle("Save Image Error");
      dlg2->Invoke();
      dlg2->Delete();
      }
    }
  
  if (strlen(path))
    {
    this->SetFileName(path);
    }
  else
    {
    this->SetFileName(NULL);
    }
  free(path);
}

void vtkKWSaveImageDialog::Create(vtkKWApplication *app, const char* /*args*/)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("SaveImageDialog already created");
    return;
    }

  this->SetApplication(app);
}


//----------------------------------------------------------------------------
void vtkKWSaveImageDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << this->GetFileName() << endl;
}
