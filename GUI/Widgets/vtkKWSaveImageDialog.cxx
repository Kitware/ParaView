/*=========================================================================

  Module:    vtkKWSaveImageDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSaveImageDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSaveImageDialog );
vtkCxxRevisionMacro(vtkKWSaveImageDialog, "1.26");

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
          this->GetApplication(), 0, "Save Image Error", 
          "A valid file extension was not found.\n"
          "Please use a .bmp, .jpg, .png, .ppm, or .tif file extension\n"
          "when naming your file.", vtkKWMessageDialog::ErrorIcon);
        }
      }
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWSaveImageDialog::Create(vtkKWApplication *app, const char* args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkKWSaveImageDialog already created");
    return;
    }

  // Call the superclass to create the dialog

  this->Superclass::Create(app, args);

  this->SetTitle("Save As Image");
}

//----------------------------------------------------------------------------
void vtkKWSaveImageDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
