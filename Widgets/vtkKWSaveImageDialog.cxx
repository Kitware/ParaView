/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSaveImageDialog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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
    vtkKWObject::Script(this->Application,
			"tk_getSaveFile -title \"Save As Image\" -filetypes {{{Windows Bitmap} {.bmp}} {{Binary PPM} {.ppm}} {{TIFF Images} {.tif}}}");
    if (path)
      {
      free(path);
      }
    path = strdup(this->Application->GetMainInterp()->result);
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
      vtkKWObject::Script(this->Application,"tk_messageBox -icon error -title \"Save Image Error\" -message \"A valid file extension was not found.\\nPlease use a .bmp, .ppm, or .tif file extension\\nwhen naming your file.\"");
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

void vtkKWSaveImageDialog::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("SaveImageDialog already created");
    return;
    }

  this->SetApplication(app);
}

