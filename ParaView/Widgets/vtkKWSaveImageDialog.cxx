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



//------------------------------------------------------------------------------
vtkKWSaveImageDialog* vtkKWSaveImageDialog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWSaveImageDialog");
  if(ret)
    {
    return (vtkKWSaveImageDialog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWSaveImageDialog;
}




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
    this->Script("tk_getSaveFile -title {Save As Image} -defaultextension {.bmp} -filetypes {{{Windows Bitmap} {.bmp}} {{Binary PPM} {.ppm}} {{TIFF Images} {.tif}}}");
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
	     !strcmp(path + strlen(path) - 4,".pnm"))
      {
      done = 1;
      }
    else
      {
      // unknown file extension
      this->Script("tk_messageBox -icon error -title \"Save Image Error\" -message \"A valid file extension was not found.\\nPlease use a .bmp, .ppm, or .tif file extension\\nwhen naming your file.\"");
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

void vtkKWSaveImageDialog::Create(vtkKWApplication *app, const char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("SaveImageDialog already created");
    return;
    }

  this->SetApplication(app);
}

