/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWSaveImageDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSaveImageDialog );
vtkCxxRevisionMacro(vtkKWSaveImageDialog, "1.20.2.1");

int vtkKWSaveImageDialogCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWSaveImageDialog::vtkKWSaveImageDialog()
{
  this->CommandFunction = vtkKWSaveImageDialogCommand;
}

//----------------------------------------------------------------------------
int vtkKWSaveImageDialog::Invoke()
{
  int res = 0;

  this->SaveDialogOn();
  this->SetFileTypes("{{Windows Bitmap} {.bmp}} "
                     "{{JPEG Images} {.jpg}} "
                     "{{PNG Images} {.png}} "
                     "{{Binary PPM} {.ppm}} "
                     "{{TIFF Images} {.tif}}");
  this->SetDefaultExtension(".bmp");

  int done = 0;
  while (!done)
    {
    if (!this->vtkKWLoadSaveDialog::Invoke())
      {
      done = 1;
      }
    else 
      {
      const char *fname = this->GetFileName();
      const char *ext = fname + strlen(fname) - 4;
      if (fname && strlen(fname) &&
          (!strcmp(ext,".bmp") ||
           !strcmp(ext,".jpg") ||
           !strcmp(ext,".png") ||
           !strcmp(ext,".ppm") ||
           !strcmp(ext,".tif")))
        {
        this->GenerateLastPath(fname);
        res = 1;
        done = 1;
        }
      else
        {
        vtkKWMessageDialog::PopupMessage( 
          this->Application, 0, "Save Image Error", 
          "A valid file extension was not found.\n"
          "Please use a .bmp, .jpg, .png, .ppm, or .tif file extension\n"
          "when naming your file.", vtkKWMessageDialog::ErrorIcon);
        }
      }
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWSaveImageDialog::Create(vtkKWApplication *app, const char* /*args*/)
{
  // Already created ?

  if (this->Application)
    {
    vtkErrorMacro("SaveDialog already created");
    return;
    }

  this->SetApplication(app);
  this->SetTitle("Save As Image");
}
